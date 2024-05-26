Filters
=======

High Pass Filter
----------------

`float apply_hpf(float input, float cutoff, float resonance, uint8_t channel)`

Processes a single audio sample by applying a high pass filter, which allows frequencies above a specified cutoff point to pass through while attenuating frequencies below this point. The function both takes and outputs a single processed PCM sample.

**Parameters:**

- `input`: The current audio sample to be processed.

- `cutoff`: This parameter sets the frequency threshold of the high pass filter. Frequencies above this threshold will pass through the filter, while those below will be attenuated. The behavior of this is non-intuitive, see *Filter Cutoff Behavior* for more information.

- `resonance`: Determines the level of emphasis and steepness of the filter's cutoff point. A **higher** resonance value results in a more pronounced and sharper cutoff, enhancing the frequencies at the cutoff point. Typically don't exceed the 0.05 - 0.95 range or it'll whine.

- `channel`: Indicates the channel number of the sample being processed. It contains internal storage to keep track of the last processed sample for each channel, which is basically always stereo audio (2 channels). This does mean it's not possible to switch voice processing per sample as it needs two samples to work.


Low Pass Filter
----------------

`float apply_lpf(float input, float cutoff, float resonance, uint8_t channel)`

Processes a single audio sample by applying a low pass filter, which allows frequencies above a specified cutoff point to pass through while attenuating frequencies below this point. The function both takes and outputs a single processed PCM sample.

**Parameters:**

- `input`: The current audio sample to be processed.

- `cutoff`: This parameter sets the frequency threshold of the high pass filter. Frequencies below this threshold will pass through the filter, while those below will be attenuated. The behavior of this is non-intuitive, see *Filter Cutoff Behavior* for more information.

- `resonance`: Determines the level of emphasis and steepness of the filter's cutoff point. A **lower** resonance value results in a more pronounced and sharper cutoff, enhancing the frequencies at the cutoff point. I've found that you want to keep it between 0.03 and roughly 0.3. You get diminishing returns beyond 0.3, and it can get *really groovy* when you go beyond 1.0.

- `channel`: Indicates the channel number of the sample being processed. It contains internal storage to keep track of the last processed sample for each channel, which is basically always stereo audio (2 channels). This does mean it's not possible to switch voice processing per sample as it needs two samples to work.

Filter Cutoff Behavior
----------------------

The cutoff curve is roooouuughly $`(x^0.57)*0.5`$, but it probably deviates pretty quickly for values beyond the tested range. I use the below as a reference. The first column refers to the range mapped from 0.001-2.0 to 1-255, used in the `apply_eq()` function.

| Input (Mapped) | Input (Float) | Cutoff Frequency (KHz) |
| -------------- | ------------- | ---------------------- |
| 1              | 0.001         | 0.50                   |
| 10             | 0.071         | 1.86                   |
| 20             | 0.151         | 2.76                   |
| 30             | 0.229         | 3.47                   |
| 40             | 0.308         | 4.09                   |
| 50             | 0.387         | 4.65                   |
| 60             | 0.465         | 5.16                   |
| 70             | 0.544         | 5.63                   |
| 80             | 0.623         | 6.08                   |
| 90             | 0.701         | 6.50                   |
| 100            | 0.780         | 6.90                   |
| 110            | 0.859         | 7.29                   |
| 120            | 0.938         | 7.66                   |
| 130            | 1.016         | 8.02                   |
| 140            | 1.094         | 8.36                   |
| 150            | 1.174         | 8.70                   |
| 160            | 1.252         | 9.02                   |
| 170            | 1.331         | 9.34                   |
| 180            | 1.410         | 9.65                   |
| 190            | 1.488         | 9.95                   |
| 200            | 1.567         | 10.25                  |
| 210            | 1.646         | 10.54                  |
| 220            | 1.725         | 10.82                  |
| 230            | 1.803         | 11.10                  |
| 240            | 1.882         | 11.37                  |
| 250            | 1.960         | 11.64                  |

EQ Band (Low Pass + High Pass)
------------------------------

`void apply_eq(float* input_buffer, size_t input_length, float low_cutoff, float high_cutoff, float low_q, float hi_q, uint8_t intensity, float mix)`

The `apply_eq` function applies both low pass and high pass filters to an input buffer, creating a band pass filter effect. This allows for targeted frequency manipulation within a specified range, aka a 1-band EQ. Generally this is more than enough, but it's possible to chain them together to get more control. If `mix` is set to 0.0, the function exits early as no processing is required. If mix exceeds 1.0, it is clamped to 1.0 to prevent invalid blending.

**Parameters:**

- `input_buffer`: A pointer to the array of audio samples to be processed. This array is modified in place.

- `input_length`: The number of audio samples in the input_buffer. This should be the total length of the array.

- `low_cutoff`: The cutoff frequency for the low-pass filter. Frequencies below this value are allowed to pass. Setting this to a value greater than 0 enables the low-pass filter.

- `high_cutoff`: The cutoff frequency for the high-pass filter. Frequencies above this value are allowed to pass. Setting this to a value greater than 0 enables the high-pass filter.

- `low_q`: The resonance parameter for the low-pass filter. *Lower* values result in a sharper cutoff at the specified frequency.

- `hi_q`: The resonance parameter for the high-pass filter. *Higher* values result in a sharper cutoff at the specified frequency.

- `intensity`: The number of times the filter processing loop is executed. Higher values increase the strength of the filtering effect, but also the processing time.

- `mix`: A float value between 0.0 and 1.0 that determines the blend between the original (dry) signal and the processed (wet) signal. A value of 0.0 means only the original signal is output, while a value of 1.0 means only the processed signal is output. Values between 0.0 and 1.0 provide a mix of both signals.

Wave Shaping
============

Tanh Wave Shaping
-----------------

`float tan_smooth(float input, float strength)`

The `tan_smooth` function is a nice little curve generator designed to transform a linear input range (0.0 - 1.0) into a non-linear output. This transformation can add timbral interest to an input waveform by modifying its shape on one side, introducing a more natural sound quality through a slight DC wobble. The degree of variation can be controlled using the `strength` parameter, which is primarily useful when applied to one channel, creating stereo differences.

The function both takes and outputs a single processed PCM sample.

**Parameters:**

- `input`: The current audio sample to be processed.

- `strength`: Adjusts the amount of variation applied to the waveform. Higher values result in more pronounced changes.


Foldback Distortion
-------------------

`float foldback(float input, float threshold)`

The `foldback` function is a fun type of distortion that works by inverting waveform data that exceeds a specified threshold - fancy clipping really. Although there's no foldback distortion decay variable, because there's a volume decay parameter in the instrument definition, as the waveform amplitude slowly drops below the foldback point it slowly becomes less distorted.

The function both takes and outputs a single processed PCM sample.

**Parameters:**

- `input`:  A float value representing the waveform data to be distorted.

- `threshold`:  A float value defining the foldback point. Waveform data above or below this threshold will be folded back on itself. Worth noting that this does not follow the volume set in the MMML data, so if you're gently clipping at v5, v4 will probably have no distortion and v6 will have loads. I've actually left this behavior as it adds timbral variation to different 'velocities'.