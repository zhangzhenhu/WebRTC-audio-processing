/*	09/2017
	shichaog
	This is the main test File. Include noise suppresion, AEC, VAD.

*/
#include <string>
#include <iostream>

//#include "webrtc/modules/audio_processing/include/audio_processing.h"
//#include "webrtc/modules/interface/module_common_types.h"
#include "release/include/webrtc_audio_processing/webrtc/modules/audio_processing/include/audio_processing.h"
#include "release/include/webrtc_audio_processing/webrtc/modules/interface/module_common_types.h"
//#include "webrtc/common_audio/wav_file.h"
#include <sndfile.h>

#define EXPECT_OP(op, val1, val2)                                       \
  do {                                                                  \
    if (!((val1) op (val2))) {                                          \
      fprintf(stderr, "Check failed: %s %s %s\n", #val1, #op, #val2);   \
      exit(1);                                                          \
    }                                                                   \
  } while (0)

#define EXPECT_EQ(val1, val2)  EXPECT_OP(==, val1, val2)
#define EXPECT_NE(val1, val2)  EXPECT_OP(!=, val1, val2)
#define EXPECT_GT(val1, val2)  EXPECT_OP(>, val1, val2)
#define EXPECT_LT(val1, val2)  EXPECT_OP(<, val1, val2)

int usage() {
    std::cout <<
              "Usage: webrtc-audio-process -anc|-agc|-aec value input.wav output.wav [delay_ms echo_in.wav]"
              << std::endl;
    return 1;
}

bool ReadFrame(FILE *file, webrtc::AudioFrame *frame) {
    // The files always contain stereo audio.
    size_t frame_size = frame->samples_per_channel_;
    size_t read_count = fread(frame->data_,
                              sizeof(int16_t),
                              frame_size,
                              file);
    if (read_count != frame_size) {
        // Check that the file really ended.
        EXPECT_NE(0, feof(file));
        return false;  // This is expected.
    }
    return true;
}

bool WriteFrame(FILE *file, webrtc::AudioFrame *frame) {
    // The files always contain stereo audio.
    size_t frame_size = frame->samples_per_channel_;
    size_t read_count = fwrite(frame->data_,
                               sizeof(int16_t),
                               frame_size,
                               file);
    if (read_count != frame_size) {
        return false;  // This is expected.
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc != 5 && argc != 7) {
        return usage();
    }

    bool is_echo_cancel = false;
    int level = -1;
    level = atoi(argv[2]);
    int delay_ms = 0;

    // Usage example, omitting error checking:
    webrtc::AudioProcessing *apm = webrtc::AudioProcessing::Create();

    webrtc::Config config;
    apm->level_estimator()->Enable(true);

    config.Set<webrtc::ExtendedFilter>(new webrtc::ExtendedFilter(true));
    config.Set<webrtc::DelayAgnostic>(new webrtc::DelayAgnostic(true));

    apm->echo_cancellation()->Enable(true);
    apm->echo_cancellation()->enable_metrics(true);
    apm->echo_cancellation()->enable_delay_logging(true);
    apm->set_stream_delay_ms(delay_ms);

    apm->echo_cancellation()->enable_drift_compensation(true);
    apm->echo_cancellation()->set_suppression_level(webrtc::EchoCancellation::kHighSuppression);

    apm->SetExtraOptions(config);

    apm->high_pass_filter()->Enable(true);
    if (std::string(argv[1]) == "-anc") {
        std::cout << "ANC: level " << level << std::endl;
        apm->noise_suppression()->Enable(true);
        switch (level) {
            case 0:
                apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kLow);
                break;
            case 1:
                apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kModerate);
                break;
            case 2:
                apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kHigh);
                break;
            case 3:
                apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kVeryHigh);
                break;
            default:
                apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kVeryHigh);
        }
        apm->voice_detection()->Enable(true);
    } else if (std::string(argv[1]) == "-agc") {
        std::cout << "AGC: model " << level << std::endl;
        apm->gain_control()->Enable(true);
        apm->gain_control()->set_analog_level_limits(0, 255);
        switch (level) {
            case 0:
                apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
                break;
            case 1:
                apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveDigital);
                break;
            case 2:
                apm->gain_control()->set_mode(webrtc::GainControl::kFixedDigital);
                break;
            default:
                apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
        }
    } else if (std::string(argv[1]) == "-aec") {
        std::cout << "AEC: level " << level << std::endl;
        switch (level) {
            case 0:
                apm->echo_cancellation()->set_suppression_level(webrtc::EchoCancellation::kLowSuppression);
                break;
            case 1:
                apm->echo_cancellation()->set_suppression_level(webrtc::EchoCancellation::kModerateSuppression);
                break;
            case 2:
                apm->echo_cancellation()->set_suppression_level(webrtc::EchoCancellation::kHighSuppression);
        }
    } else if (std::string(argv[1]) == "-vad") {
        std::cout << "AEC: level " << level << std::endl;
        apm->voice_detection()->Enable(true);
        switch (level) {
            case 0:
                apm->voice_detection()->set_likelihood(webrtc::VoiceDetection::kVeryLowLikelihood);
                apm->voice_detection()->set_frame_size_ms(10);
        }

    } else {

        delete apm;
        return usage();
    }

    webrtc::AudioFrame far_frame;
    webrtc::AudioFrame near_frame;


    float frame_step = 10;  // ms
    int near_read_bytes;

    far_frame.sample_rate_hz_ = 16000;
    far_frame.samples_per_channel_ = far_frame.sample_rate_hz_ * frame_step / 1000.0;
    far_frame.num_channels_ = 1;
    near_frame.sample_rate_hz_ = 16000;
    near_frame.samples_per_channel_ = near_frame.sample_rate_hz_ * frame_step / 1000.0;
    near_frame.num_channels_ = 1;

    size_t size = near_frame.samples_per_channel_;

//    FILE *far_file = fopen(argv[3], "rb");
//    FILE *near_file = fopen(argv[4], "rb");
//    FILE *aec_out_file = fopen(argv[5], "wb");
    int drift_samples = 0;

    size_t read_count = 0;
    /* wave 文件存储的数据类型（整型 or 浮点型）可以判断
     * http://www.mega-nerd.com/libsndfile/api.html#open
     * 对于两个声道的立体声，如果采样位宽是int16，那么每个frame（采样sample）是两个int16，每个声道各一个。
     * 这和网上很多帖子说的不一样，这是从一个立体声文件验证后的结果。
     */
    SF_INFO sf_info;
    sf_info.format = 0;
    sf_info.samplerate = 0;
    sf_info.frames = 0;
    SNDFILE *sf_in = sf_open(argv[3], SFM_READ, &sf_info);

    std::cout << "=====in file======" << std::endl;
    std::cout << "samplerate:" << sf_info.samplerate << " format:" << sf_info.format;
    std::cout << " channels:" << sf_info.channels << " frames:" << sf_info.frames;
    std::cout << " sections:" << sf_info.sections << " seekable:" << sf_info.seekable << std::endl;
//    short *frames_in = new short[sf_info.frames];
    near_frame.sample_rate_hz_ = sf_info.samplerate;
    near_frame.num_channels_ = sf_info.channels;
    near_frame.samples_per_channel_ = near_frame.sample_rate_hz_ * frame_step / 1000.0;

    SNDFILE *sf_out = sf_open(argv[4], SFM_WRITE, &sf_info);
    int hehe=0;
    while (true) {
        /*
         * 注意：sf_readf_* 是以frame为单位进行读取数据，一个frame是包含了所有声道的一次采样。
         * 也就是说对于两个声道的文件，一个frame是包含两个声道的一次采样数据。
         */
        sf_count_t result = sf_readf_short(sf_in, near_frame.data_, near_frame.samples_per_channel_);
        if (result <= 0)
            break;
        hehe += result;
//        std::cout << "result:"<<result << std::endl;
        near_frame.samples_per_channel_ = result;
//		read_count = fread(far_frame.data_, sizeof(int16_t), size, far_file);


//		if(read_count != size){
//			fseek(near_file, read_count * sizeof(int16_t), SEEK_CUR);
//		}
//		apm->ProcessReverseStream(&far_frame);

//        webrtc::WavReader wave_file(argv[3]);


//        sf_count_t frames =


        // 只处理一个channel

//        float *frames_out = new float[sf_info.frames];

//        std::cout << "read " << result << std::endl;
//        webrtc::StreamConfig sc_in(sf_info.samplerate, 1);
//        webrtc::StreamConfig sc_out(sf_info.samplerate, 1);


//        read_count = fread(near_frame.data_, sizeof(int16_t), size, near_file);
//        near_read_bytes += read_count * sizeof(int16_t);

//        if (read_count != size) {
//            break;  // This is expected.
//        }

//        apm->set_stream_delay_ms(delay_ms);
//        std::cout << "!!!!!!!!!!drift_samples"<< drift_samples << std::endl;
//        apm->echo_cancellation()->set_stream_drift_samples(drift_samples);

        int err = apm->ProcessStream(&near_frame);
//        int err = apm->ProcessStream((const float *const *) frames_in, sc_in, sc_out, (float *const *) frames_out);
        int vad_flag = (int) apm->voice_detection()->stream_has_voice();


//        WriteFrame(aec_out_file, &near_frame);
        /*
         * 注意： near_frame.data_ 中实际数据长度是 near_frame.samples_per_channel_*near_frame.num_channels_
         */
        sf_write_short(sf_out, near_frame.data_,near_frame.samples_per_channel_*near_frame.num_channels_);
//        free(frames_in);
//        free(frames_out);

//        break;

    }
    sf_close(sf_in);
    sf_close(sf_out);
//    fclose(far_file);
//    fclose(near_file);
//    fclose(aec_out_file);
    std::cout<<hehe;
    delete apm;

    return 0;
}
