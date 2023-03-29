/**************************************************************
*  FILENAME:     alsa-playback.c
*  DESCRIPTION:  Live playback on (some) Linux devices. In the
*                case of this program, it'll work out the box
*                on Raspberry Pi OS.
*
*                Really, it'd be more computationally sensible
*                to generate each frame and write to a ring
*                buffer, for actual realtime playback. All this
*                practically does is just play a wave file from
*                RAM.
*
*  DATE:         13th March 2022
**************************************************************/

#define ALSA_SAMPLE_RATE     44100
#define ALSA_LATENCY         500000 // 0.5 seconds
#define ALSA_TOTAL_CHANNELS  2

// don't do this; there's no error checking in here which
// could be problematic for troubleshooting

void alsa_playback(int16_t *input_buffer, uint32_t input_length)
{
	static char *device = "default"; // playback device
	snd_pcm_t *handle;
	snd_pcm_sframes_t frames;

	snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
	snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, ALSA_TOTAL_CHANNELS, ALSA_SAMPLE_RATE, 1, ALSA_LATENCY);

	frames = snd_pcm_writei(handle, input_buffer, input_length / ALSA_TOTAL_CHANNELS);

	#ifdef DEBUG
		printf("Total processed frames: %d\n", frames);
		printf("Playback complete. Bye!\n");
	#endif
}