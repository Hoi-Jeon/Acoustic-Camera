#ifndef SOUND_CAMERA_H_
#define SOUND_CAMERA_H_

// Arrays for math operations
#include <valarray>   
// Input/output streams and functions
#include <iostream>   
// Complex data type
#include <complex>    
// Qt data type (e.g. QVector, ...)
#include <QtWidgets>  

static const double_t MIC_coordinates[][2] = {
	{-48.5036755, -20.0908795},
	{-48.5036755,  20.0908795},
	{-20.0908795,  48.5036755},
	{ 20.0908795,  48.5036755},
	{ 48.5036755,  20.0908795},
	{ 48.5036755, -20.0908795},
	{ 20.0908795, -48.5036755},
	{-20.0908795, -48.5036755}};

const uint16_t SamplingFrequency = 48000;
const double_t MaxAzimuthalAngle = 53.50;
const double_t MaxPolarAngle = 41.41;

const uint16_t NumberAzimuthalAngle = 21;
const uint16_t NumberPolarAngle = 16;


const double_t pi = std::acos(-1);

class SoundCamera {
public:
	// Constructor:
	SoundCamera(uint32_t sampling_rate, int32_t number_of_samples, uint16_t number_of_channels);
	// Destructor:
	~SoundCamera();
	// Read microphone data from buffer
	int16_t ReadMicrophoneData(uint32_t sample, uint16_t channel) {
		return (buffer_[sample][channel]); }
	// Write microphone data into buffer
	void WriteMicrophoneData(uint32_t sample, uint16_t channel, int16_t data) {
		buffer_[sample][channel] = data; }
	// Do Delay-and-sum Beamforming in the time-domain
	double_t BeamFormingTime(int16_t Nr_Azimuthal, int16_t Nr_Polar);
	// Do Delay-and-sum Beamforming in the frequency-domain
	double_t BeamFormingFrequency(int16_t Nr_Azimuthal, int16_t Nr_Polar);
	// Set a threshold of beamformed Max. SPL [dB]
	void SetThresholdLevel(uint16_t Level) {
		threshold_level_ = Level; }	
	// Get a threshold of beamformed Max. SPL [dB]
	uint16_t GetThresholdLevel() {
		return(threshold_level_); }	
	// Set an alpha value for exponential averagain: (0<-Slow, 1<-Fast)
	void SetAverageAlpha(double_t Level) {
		alpha_ = Level; }	
	// Perform FFT data in buffer
	void FFT_DataInBuffer();	
	// Get Frequency Data
	void GetFFT_Frequency(QVector<double_t> &Frequency);	
	// Get FFT dB Magnitude Data at a given channel
	void GetFFT_dB_Data(QVector<double_t> &Data, uint16_t channel);	

private:
	// Array for the Azimuthal angle in radian unit
	std::valarray<double_t> Angle_azimuthal_;
	// Array for the polar angle in radian unit
	std::valarray<double_t> Angle_polar_;
	// Frequency data
	std::valarray<double_t> Frequency_;	
	// 4D array for the phase delay at each Microphone [Frequency][Mic][Azimuthal angle][Polar angle]
	double_t**** phase_delay_;
	// FFT frequency data [Frequency][Channel]
	std::complex<double_t>** FFT_buffer_;
	// FFT dB magnitude only frequency data [Frequency][Channel]
	double_t** FFT_dB_buffer_;
	// Buffer for microphone time data
	int16_t** buffer_;
	// Beamforming data
	double_t** beamformed_data_;
	// 3D array for the time delay at each Microphone [Mic][Azimuthal angle][Polar angle]
	double_t*** time_delay_;
	// 3D array for the time delay at each Microphone [Mic][Azimuthal angle][Polar angle]
	int16_t*** time_delay_index_;
	// 2D array for the max time delay index
	int16_t** time_delay_index_max_;
	// 2D array for the min time delay index
	int16_t** time_delay_index_min_;
	// Smoothing factor of exponential smoothing
	double_t alpha_;
	// sampling frequency
	uint32_t sampling_rate_;
	// a number of data in one block for each channel (512 is the default value of Matrix creator)
	uint32_t number_of_samples_;
	// a number of microphones (8 is fixed for Matrix creator)
	uint16_t number_of_channels_;
	// a threshold of beamformed Max. SPL [dB]
	uint16_t threshold_level_;		

/*
 * ref: https://stackoverflow.com/questions/23776784/use-of-operator-before-a-function-name-in-c/23777436#:~:text=In%20C%2B%2B%2C%20when%20the,return%20an%20int%20by%20reference.
 *
 * In C++, when the ref-sign (&) is used before the function name in the declaration of a function
 * it is associated with the return value of the function and means that the function will return by reference.
 *
 */

};
#endif  // SOUND_CAMERA_H_
