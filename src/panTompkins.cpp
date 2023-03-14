#include "panTompkins.h"

#include <fstream>
#include <numeric>

// Sampling frequency.
constexpr int fs = 360;

// Integrator window size, in samples. The article recommends 150ms. So,
// fs*0.15. However, you should check empirically if the waveform looks ok.
constexpr int window_size = fs * 0.15;

// An indicator that there are no more samples to read. Use an
// impossible value for a sample.
constexpr float no_sample = std::numeric_limits<float>::infinity();

// The size of the buffers (in samples). Must fit more than 1.66 times an
// RR interval, which typically could be around 1 second.
constexpr int buffer_size = fs * 2;

// Delay introduced by the filters. Filter only output samples after this
// one. Set to 0 if you want to keep the delay. Fixing the delay results
// in delay less samples in the final end result.
constexpr int delay = 22;

std::ifstream fin;
std::ofstream fout;

/*
    Use this function for any kind of setup you need before getting samples.
    This is a good place to open a file, initialize your hardware and/or open
    a serial connection.
    Remember to update its parameters on the panTompkins.h file as well.
*/
void init(const char* file_in, const char* file_out) {
  fin = std::ifstream(file_in);
  fout = std::ofstream(file_out);
}

/*
    Use this function to read and return the next sample (from file, serial,
    A/D converter etc) and put it in a suitable, numeric format. Return the
    sample, or no_sample if there are no more samples.
*/
float input() {
  float num = no_sample;
  fin >> num;
  return num;
}

/*
    Use this function to output the information you see fit (last RR-interval,
    sample index which triggered a peak detection, whether each sample was a R
    peak (1) or not (0) etc), in whatever way you see fit (write on screen,
   write on file, blink a LED, call other functions to do other kinds of
   processing, such as feature extraction etc). Change its parameters to receive
   the necessary information to output.
*/
void output(int out) { fout << out << std::endl; }

/*
    This is the actual QRS-detecting function. It's a loop that constantly calls
   the input and output functions and updates the thresholds and averages until
   there are no more samples. More details both above and in shorter comments
   below.
*/
void panTompkins() {
  // The signal array is where the most recent samples are kept. The other
  // arrays are the outputs of each filtering module: DC Block, low pass, high
  // pass, integral etc. The output is a buffer where we can change a previous
  // result (using a back search) before outputting.
  float signal[buffer_size], dcblock[buffer_size], lowpass[buffer_size],
      highpass[buffer_size], derivative[buffer_size], squared[buffer_size],
      integral[buffer_size], outputSignal[buffer_size];

  // rr1 holds the last 8 RR intervals. rr2 holds the last 8 RR intervals
  // between rrlow and rrhigh. rravg1 is the rr1 average, rr2 is the rravg2.
  // rrlow = 0.92*rravg2, rrhigh = 1.08*rravg2 and rrmiss = 1.16*rravg2. rrlow
  // is the lowest RR-interval considered normal for the current heart beat,
  // while rrhigh is the highest. rrmiss is the longest that it would be
  // expected until a new QRS is detected. If none is detected for such a long
  // interval, the thresholds must be adjusted.
  int rr1[8], rr2[8], rravg1 = 0, rravg2 = 0, rrlow = 0, rrhigh = 0, rrmiss = 0;

  // i and j are iterators for loops.
  // sample counts how many samples have been read so far.
  // lastQRS stores which was the last sample read when the last R sample was
  // triggered. lastSlope stores the value of the squared slope when the last R
  // sample was triggered. currentSlope helps calculate the max. square slope
  // for the present sample. These are all long unsigned int so that very long
  // signals can be read without messing the count.
  long unsigned int i, j, sample = 0, lastQRS = 0, lastSlope = 0,
                          currentSlope = 0;

  // This variable is used as an index to work with the signal buffers. If the
  // buffers still aren't completely filled, it shows the last filled position.
  // Once the buffers are full, it'll always show the last position, and new
  // samples will make the buffers shift, discarding the oldest sample and
  // storing the newest one on the last position.
  int current;

  // There are the variables from the original Pan-Tompkins algorithm.
  // The ones ending in _i correspond to values from the integrator.
  // The ones ending in _f correspond to values from the
  // DC-block/low-pass/high-pass filtered signal. The peak variables are peak
  // candidates: signal values above the thresholds. The threshold 1 variables
  // are the threshold variables. If a signal sample is higher than this
  // threshold, it's a peak. The threshold 2 variables are half the threshold 1
  // ones. They're used for a back search when no peak is detected for too long.
  // The spk and npk variables are, respectively, running estimates of signal
  // and noise peaks.
  float peak_i = 0, peak_f = 0, threshold_i1 = 0, threshold_i2 = 0,
        threshold_f1 = 0, threshold_f2 = 0, spk_i = 0, spk_f = 0, npk_i = 0,
        npk_f = 0;

  // qrs tells whether there was a detection or not.
  // regular tells whether the heart pace is regular or not.
  // prevRegular tells whether the heart beat was regular before the newest
  // RR-interval was calculated.
  bool qrs, regular = true, prevRegular;

  // Initializing the RR averages
  for (i = 0; i < 8; i++) {
    rr1[i] = 0;
    rr2[i] = 0;
  }

  // The main loop where everything proposed in the paper happens. Ends when
  // there are no more signal samples.
  do {
    // Test if the buffers are full.
    // If they are, shift them, discarding the oldest sample and adding the new
    // one at the end. Else, just put the newest sample in the next free
    // position. Update 'current' so that the program knows where's the newest
    // sample.
    if (sample >= buffer_size) {
      for (i = 0; i < buffer_size - 1; i++) {
        signal[i] = signal[i + 1];
        dcblock[i] = dcblock[i + 1];
        lowpass[i] = lowpass[i + 1];
        highpass[i] = highpass[i + 1];
        derivative[i] = derivative[i + 1];
        squared[i] = squared[i + 1];
        integral[i] = integral[i + 1];
        outputSignal[i] = outputSignal[i + 1];
      }
      current = buffer_size - 1;
    } else {
      current = sample;
    }
    signal[current] = input();

    // If no sample was read, stop processing!
    if (signal[current] == no_sample) break;
    sample++;  // Update sample counter

    // DC Block filter
    // This was not proposed on the original paper.
    // It is not necessary and can be removed if your sensor or database has no
    // DC noise.
    if (current >= 1)
      dcblock[current] =
          signal[current] - signal[current - 1] + 0.995 * dcblock[current - 1];
    else
      dcblock[current] = 0;

    // Low Pass filter
    // Implemented as proposed by the original paper.
    // y(nT) = 2y(nT - T) - y(nT - 2T) + x(nT) - 2x(nT - 6T) + x(nT - 12T)
    // Can be removed if your signal was previously filtered, or replaced by a
    // different filter.
    lowpass[current] = dcblock[current];
    if (current >= 1) lowpass[current] += 2 * lowpass[current - 1];
    if (current >= 2) lowpass[current] -= lowpass[current - 2];
    if (current >= 6) lowpass[current] -= 2 * dcblock[current - 6];
    if (current >= 12) lowpass[current] += dcblock[current - 12];

    // High Pass filter
    // Implemented as proposed by the original paper.
    // y(nT) = 32x(nT - 16T) - [y(nT - T) + x(nT) - x(nT - 32T)]
    // Can be removed if your signal was previously filtered, or replaced by a
    // different filter.
    highpass[current] = -lowpass[current];
    if (current >= 1) highpass[current] -= highpass[current - 1];
    if (current >= 16) highpass[current] += 32 * lowpass[current - 16];
    if (current >= 32) highpass[current] += lowpass[current - 32];

    // Derivative filter
    // This is an alternative implementation, the central difference method.
    // f'(a) = [f(a+h) - f(a-h)]/2h
    // The original formula used by Pan-Tompkins was:
    // y(nT) = (1/8T)[-x(nT - 2T) - 2x(nT - T) + 2x(nT + T) + x(nT + 2T)]
    derivative[current] = highpass[current];
    if (current > 0) derivative[current] -= highpass[current - 1];

    // This just squares the derivative, to get rid of negative values and
    // emphasize high frequencies. y(nT) = [x(nT)]^2.
    squared[current] = derivative[current] * derivative[current];

    // Moving-Window Integration
    // Implemented as proposed by the original paper.
    // y(nT) = (1/N)[x(nT - (N - 1)T) + x(nT - (N - 2)T) + ... x(nT)]
    // window_size, in samples, must be defined so that the window is ~150ms.

    integral[current] = 0;
    for (i = 0; i < window_size; i++) {
      if (current >= (float)i)
        integral[current] += squared[current - i];
      else
        break;
    }
    integral[current] /= (float)i;

    qrs = false;

    // If the current signal is above one of the thresholds (integral or
    // filtered signal), it's a peak candidate.
    if (integral[current] >= threshold_i1 ||
        highpass[current] >= threshold_f1) {
      peak_i = integral[current];
      peak_f = highpass[current];
    }

    // If both the integral and the signal are above their thresholds, they're
    // probably signal peaks.
    if ((integral[current] >= threshold_i1) &&
        (highpass[current] >= threshold_f1)) {
      // There's a 200ms latency. If the new peak respects this condition, we
      // can keep testing.
      if (sample > lastQRS + fs / 5) {
        // If it respects the 200ms latency, but it doesn't respect the 360ms
        // latency, we check the slope.
        if (sample <= lastQRS + (long unsigned int)(0.36 * fs)) {
          // The squared slope is "M" shaped. So we have to check nearby samples
          // to make sure we're really looking at its peak value, rather than a
          // low one.
          currentSlope = 0;
          for (j = current - 10; j <= current; j++)
            if (squared[j] > currentSlope) currentSlope = squared[j];

          if (currentSlope <= (float)(lastSlope / 2)) {
            qrs = false;
          }

          else {
            spk_i = 0.125 * peak_i + 0.875 * spk_i;
            threshold_i1 = npk_i + 0.25 * (spk_i - npk_i);
            threshold_i2 = 0.5 * threshold_i1;

            spk_f = 0.125 * peak_f + 0.875 * spk_f;
            threshold_f1 = npk_f + 0.25 * (spk_f - npk_f);
            threshold_f2 = 0.5 * threshold_f1;

            lastSlope = currentSlope;
            qrs = true;
          }
        }
        // If it was above both thresholds and respects both latency periods, it
        // certainly is a R peak.
        else {
          currentSlope = 0;
          for (j = current - 10; j <= current; j++)
            if (squared[j] > currentSlope) currentSlope = squared[j];

          spk_i = 0.125 * peak_i + 0.875 * spk_i;
          threshold_i1 = npk_i + 0.25 * (spk_i - npk_i);
          threshold_i2 = 0.5 * threshold_i1;

          spk_f = 0.125 * peak_f + 0.875 * spk_f;
          threshold_f1 = npk_f + 0.25 * (spk_f - npk_f);
          threshold_f2 = 0.5 * threshold_f1;

          lastSlope = currentSlope;
          qrs = true;
        }
      }
      // If the new peak doesn't respect the 200ms latency, it's noise. Update
      // thresholds and move on to the next sample.
      else {
        peak_i = integral[current];
        npk_i = 0.125 * peak_i + 0.875 * npk_i;
        threshold_i1 = npk_i + 0.25 * (spk_i - npk_i);
        threshold_i2 = 0.5 * threshold_i1;
        peak_f = highpass[current];
        npk_f = 0.125 * peak_f + 0.875 * npk_f;
        threshold_f1 = npk_f + 0.25 * (spk_f - npk_f);
        threshold_f2 = 0.5 * threshold_f1;
        qrs = false;
        outputSignal[current] = qrs;
        if (sample > delay + buffer_size) output(outputSignal[0]);
        continue;
      }
    }

    // If a R-peak was detected, the RR-averages must be updated.
    if (qrs) {
      // Add the newest RR-interval to the buffer and get the new average.
      rravg1 = 0;
      for (i = 0; i < 7; i++) {
        rr1[i] = rr1[i + 1];
        rravg1 += rr1[i];
      }
      rr1[7] = sample - lastQRS;
      lastQRS = sample;
      rravg1 += rr1[7];
      rravg1 *= 0.125;

      // If the newly-discovered RR-average is normal, add it to the "normal"
      // buffer and get the new "normal" average. Update the "normal" beat
      // parameters.
      if ((rr1[7] >= rrlow) && (rr1[7] <= rrhigh)) {
        rravg2 = 0;
        for (i = 0; i < 7; i++) {
          rr2[i] = rr2[i + 1];
          rravg2 += rr2[i];
        }
        rr2[7] = rr1[7];
        rravg2 += rr2[7];
        rravg2 *= 0.125;
        rrlow = 0.92 * rravg2;
        rrhigh = 1.16 * rravg2;
        rrmiss = 1.66 * rravg2;
      }

      prevRegular = regular;
      if (rravg1 == rravg2) {
        regular = true;
      }
      // If the beat had been normal but turned odd, change the thresholds.
      else {
        regular = false;
        if (prevRegular) {
          threshold_i1 /= 2;
          threshold_f1 /= 2;
        }
      }
    }
    // If no R-peak was detected, it's important to check how long it's been
    // since the last detection.
    else {
      // If no R-peak was detected for too long, use the lighter thresholds and
      // do a back search. However, the back search must respect the 200ms limit
      // and the 360ms one (check the slope).
      if ((sample - lastQRS > (long unsigned int)rrmiss) &&
          (sample > lastQRS + fs / 5)) {
        for (i = current - (sample - lastQRS) + fs / 5;
             i < (long unsigned int)current; i++) {
          if ((integral[i] > threshold_i2) && (highpass[i] > threshold_f2)) {
            currentSlope = 0;
            for (j = i - 10; j <= i; j++)
              if (squared[j] > currentSlope) currentSlope = squared[j];

            if ((currentSlope < (float)(lastSlope / 2)) &&
                (i + sample) < lastQRS + 0.36 * lastQRS) {
              qrs = false;
            } else {
              peak_i = integral[i];
              peak_f = highpass[i];
              spk_i = 0.25 * peak_i + 0.75 * spk_i;
              spk_f = 0.25 * peak_f + 0.75 * spk_f;
              threshold_i1 = npk_i + 0.25 * (spk_i - npk_i);
              threshold_i2 = 0.5 * threshold_i1;
              lastSlope = currentSlope;
              threshold_f1 = npk_f + 0.25 * (spk_f - npk_f);
              threshold_f2 = 0.5 * threshold_f1;
              // If a signal peak was detected on the back search, the RR
              // attributes must be updated. This is the same thing done when a
              // peak is detected on the first try.
              // RR Average 1
              rravg1 = 0;
              for (j = 0; j < 7; j++) {
                rr1[j] = rr1[j + 1];
                rravg1 += rr1[j];
              }
              rr1[7] = sample - (current - i) - lastQRS;
              qrs = true;
              lastQRS = sample - (current - i);
              rravg1 += rr1[7];
              rravg1 *= 0.125;

              // RR Average 2
              if ((rr1[7] >= rrlow) && (rr1[7] <= rrhigh)) {
                rravg2 = 0;
                for (i = 0; i < 7; i++) {
                  rr2[i] = rr2[i + 1];
                  rravg2 += rr2[i];
                }
                rr2[7] = rr1[7];
                rravg2 += rr2[7];
                rravg2 *= 0.125;
                rrlow = 0.92 * rravg2;
                rrhigh = 1.16 * rravg2;
                rrmiss = 1.66 * rravg2;
              }

              prevRegular = regular;
              if (rravg1 == rravg2) {
                regular = true;
              } else {
                regular = false;
                if (prevRegular) {
                  threshold_i1 /= 2;
                  threshold_f1 /= 2;
                }
              }

              break;
            }
          }
        }

        if (qrs) {
          outputSignal[current] = false;
          outputSignal[i] = true;
          if (sample > delay + buffer_size) output(outputSignal[0]);
          continue;
        }
      }

      // Definitely no signal peak was detected.
      if (!qrs) {
        // If some kind of peak had been detected, then it's certainly a noise
        // peak. Thresholds must be updated accordinly.
        if ((integral[current] >= threshold_i1) ||
            (highpass[current] >= threshold_f1)) {
          peak_i = integral[current];
          npk_i = 0.125 * peak_i + 0.875 * npk_i;
          threshold_i1 = npk_i + 0.25 * (spk_i - npk_i);
          threshold_i2 = 0.5 * threshold_i1;
          peak_f = highpass[current];
          npk_f = 0.125 * peak_f + 0.875 * npk_f;
          threshold_f1 = npk_f + 0.25 * (spk_f - npk_f);
          threshold_f2 = 0.5 * threshold_f1;
        }
      }
    }
    // The current implementation outputs '0' for every sample where no peak was
    // detected, and '1' for every sample where a peak was detected. It should
    // be changed to fit the desired application. The 'if' accounts for the
    // delay introduced by the filters: we only start outputting after the
    // delay. However, it updates a few samples back from the buffer. The reason
    // is that if we update the detection for the current sample, we might miss
    // a peak that could've been found later by backsearching using lighter
    // thresholds. The final waveform output does match the original signal,
    // though.
    outputSignal[current] = qrs;
    if (sample > delay + buffer_size) output(outputSignal[0]);
  } while (signal[current] != no_sample);

  // Output the last remaining samples on the buffer
  for (i = 1; i < buffer_size; i++) output(outputSignal[i]);
}
