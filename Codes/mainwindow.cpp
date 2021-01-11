#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

// System calls
#include <unistd.h>
// Input/output streams and functions
#include <iostream>
// Included for sin() function.
#include <cmath>
// Google gflags parser
#include <gflags/gflags.h>
// Input/output stream class to operate on files
#include <fstream>
// Input/output streams and functions
#include <iostream>
// Use strings
#include <string>
// Arrays for math operations
#include <valarray>

// Interfaces with Everloop
#include "matrix_hal/everloop.h"
// Holds data for Everloop
#include "matrix_hal/everloop_image.h"
// Communicates with MATRIX device
#include "matrix_hal/matrixio_bus.h"
// Interfaces with microphone array
#include "matrix_hal/microphone_array.h"
// Enables using FIR filter with microphone array
#include "matrix_hal/microphone_core.h"

// External libaray: QCustomPlot
#include "qcustomplot.h"
// External library: Raspicam
#include <raspicam/raspicam.h>
// External library: FFTW3
#include <fftw3.h>
// External library: sound_camera
#include "sound_camera.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // Initialize GUI Widgets
    ui->setupUi(this);

    // Button Update
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);
    ui->btnClose->setEnabled(true);
    ui->radioButtonTime->setChecked(true);

    // Define QTimer
    MainWindow::dataTimer = new QTimer(this);

    // Initialize MatrixIOBus object bus and exit program if error occurs
    if (!bus.Init()) close();

    // Set-up MicophoneArray
    microphone_array.Setup(&bus);
    microphone_array.SetSamplingRate(SamplingFrequency);
    // Calculate and set up beamforming delays for beamforming (default)
    microphone_array.CalculateDelays(0, 0, 1000, 344 * 1000);

    // Create Sound_Camera class
    sound_camera = new SoundCamera( microphone_array.SamplingRate(),
                                    microphone_array.NumberOfSamples(),
                                    microphone_array.Channels() );

    // Initialize the data in the colormap
    colorMap = new QCPColorMap(ui->customPlot->xAxis, ui->customPlot->yAxis);
    // Set whether the color map image shall use bicubic interpolation
    colorMap->setInterpolate(true);
    // Set whether the outer most data rows and columns are clipped to the specified key and value range
    colorMap->setTightBoundary(true);
    // Add a color scale:
    QCPColorScale *colorScale = new QCPColorScale(ui->customPlot);
    // Add it to the right of the main axis rect
    ui->customPlot->plotLayout()->addElement(0, 1, colorScale);
    // Scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    colorScale->setType(QCPAxis::atRight);
    // Associate the color map with the color scale
    colorMap->setColorScale(colorScale);
    // Set the lable of colormap
    colorScale->axis()->setLabel("Sound pressure level [dB]");
    // Set the precision of the tick label numbers
    colorScale->axis()->setNumberPrecision(2);

    // Colormap Gradient and Transparency alpha    
    /* Jet colormap: self-made */
    QCPColorGradient jetGradient;
    jetGradient.clearColorStops();
    jetGradient.setColorStopAt(0.00, QColor(  0,   0, 127,   0));
    jetGradient.setColorStopAt(0.55, QColor(  0,   0, 127,  20));
    jetGradient.setColorStopAt(0.60, QColor(  0,   0, 127,  90));
    jetGradient.setColorStopAt(0.65, QColor(  0,   0, 255,  90));
    jetGradient.setColorStopAt(0.70, QColor(  0, 127, 255, 120));
    jetGradient.setColorStopAt(0.75, QColor(  0, 255, 255, 150));
    jetGradient.setColorStopAt(0.80, QColor(127, 255, 127, 180));
    jetGradient.setColorStopAt(0.85, QColor(255, 255,   0, 180));
    jetGradient.setColorStopAt(0.90, QColor(255, 127,   0, 180));
    jetGradient.setColorStopAt(0.95, QColor(255,   0,   0, 150));
    jetGradient.setColorStopAt(1.00, QColor(127,   0,   0, 120));
    colorMap->setGradient(jetGradient);    

    // Initialize tha bargraph, FFTPlot
    FFTbar = new QCPBars(ui->FFTPlot->xAxis, ui->FFTPlot->yAxis);
    FFTbar->setBrush(QColor(255, 0, 0, 250));
    FFTbar->setWidth(40);
    // Give the axes some labels
    ui->FFTPlot->xAxis->setLabel("Frequency [Hz]");
    ui->FFTPlot->yAxis->setLabel("Magnitude [dB]");
    // Set axes ranges    
    ui->FFTPlot->xAxis->setRange(0, microphone_array.SamplingRate()/5);
    //ui->FFTPlot->xAxis->setRange(0, microphone_array.SamplingRate()/2);
    ui->FFTPlot->yAxis->setRange(0, 90);
    //FFTbar->setBaseValue(0); // use "setBaseValue", when the most bars in graph get negative    
}

MainWindow::~MainWindow()
{
    // Close SoundCamera class
    delete sound_camera;
    // Close GUI
    delete ui;
}

void MainWindow::realtimeDataSlot()
{
    //
    // * Read microphone stream data
    //
    microphone_array.Read();
    // Counter variable for tracking recording time
    uint32_t samples = 0;
    // For number of samples
    for (uint32_t s = 0; s < microphone_array.NumberOfSamples(); s++) {
        // For each microphone
        for (uint16_t c = 0; c < microphone_array.Channels(); c++) {
          // Write microphone data to buffer
          sound_camera->WriteMicrophoneData(s, c, microphone_array.At(s, c));
        }
        // Writes the beamformed microphone data into buffer
        sound_camera->WriteMicrophoneData(s, microphone_array.Channels(), microphone_array.Beam(s));
        // Increment samples for buffer write
        samples++;
    }
    
    //
    // * Do FFT on the data in buffer
    //    
    sound_camera->FFT_DataInBuffer();

    //
    // * Update the FFT Plot
    //
    if (ui->ckbFFT->isChecked()) {
        QVector<double> x(microphone_array.NumberOfSamples()), y(microphone_array.NumberOfSamples()); 
        // Read Frequency and FFT Magnitude
        sound_camera->GetFFT_Frequency(x);
        sound_camera->GetFFT_dB_Data(y, microphone_array.Channels());
        // Plot FFT Data
        FFTbar->setData(x, y);
        ui->FFTPlot->replot();
    }

    //
    // * Perform delay-and-sum beamforming
    // * Update the colormap
    //
    // Find the max. peak value of beamforming data, for a display showing a valuve over threshold
    double_t tmpSPL_Max=0.0;
    double_t tmpSPL=0.0;
    
    // Sets the data of the cell with indices keyIndex and valueIndex to z
    for (uint32_t x=0; x<NumberAzimuthalAngle; ++x)
        for (uint32_t y=0; y<NumberPolarAngle; ++y) {
            
            // Time-domain beamforming
            if (ui->radioButtonTime->isChecked()) {
                tmpSPL = sound_camera->BeamFormingTime(x, y);
            } else {
                // Frequency-domain beamforming
                tmpSPL = sound_camera->BeamFormingFrequency(x, y);
            }
            
            if (tmpSPL > tmpSPL_Max) tmpSPL_Max = tmpSPL;
            colorMap->data()->setCell(x, y, tmpSPL);
        }

    uint16_t Threshold_Level = sound_camera->GetThresholdLevel();
    if (tmpSPL_Max < Threshold_Level)
        // Set Data Range for not-displaying the colormap, in case tmpSPL_Max is too low
        colorMap->setDataRange(QCPRange(Threshold_Level, Threshold_Level+5));
    else
        // Rescale Data Range
        colorMap->rescaleDataRange(true);

    // Rescale Axes
    ui->customPlot->rescaleAxes();
    // Replot
    ui->customPlot->replot();
}

void MainWindow::on_btnStart_clicked()
{
    //
    // * Button Update
    //
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);
    ui->btnClose->setEnabled(false);

    //
    // * Matrix_hal
    //
    // Create MicrophoneCore object
    matrix_hal::MicrophoneCore microphone_core(microphone_array);
    // Set microphone_core to use MatrixIOBus bus
    microphone_core.Setup(&bus);

    //
    // * Colormap
    //
    // Set the number of calculation points
    colorMap->data()->setSize(NumberAzimuthalAngle, NumberPolarAngle);
    // Set range for the horizontal and vertial field of view of RPI Camera Module V1
    //  - Horizontal field of view: 53.50 +/- 0.13 degrees
    //  - Vertical field of view: 41.41 +/- 0.11 degrees
    colorMap->data()->setRange(QCPRange(-MaxAzimuthalAngle/2., MaxAzimuthalAngle/2.),
                                QCPRange(-MaxPolarAngle/2., MaxPolarAngle/2.));

    //
    // * Timer
    //
    // Set up a timer that repeatedly calls MainWindow::realtimeDataSlot:
    connect(dataTimer, SIGNAL(timeout()), this, SLOT(realtimeDataSlot()));
    dataTimer->start(0); // Interval 0 means to refresh as fast as possible

    //
    // * Camera
    //
    // Set Camera screen size
    camera.setWidth(320);
    camera.setHeight(240);
    unsigned char* data = new unsigned char[camera.getImageTypeSize(raspicam::RASPICAM_FORMAT_RGB)];
    // Open camera
    camera.open();
    // Camera setting
    camera.setCaptureSize(320, 240);
    camera.setBrightness(50); // [0 100]
    camera.setSharpness(0); // [-100 100]
    camera.setContrast(0); // [-100 100]
    // Set the background image from camera in customPlot
    ui->customPlot->axisRect()->setBackgroundScaled(true);
    ui->customPlot->axisRect()->setBackgroundScaledMode(Qt::AspectRatioMode::IgnoreAspectRatio);
    // Turn on the flag for the camera
    IsCameraRunning = true;

    // Capture camera photos
    while(IsCameraRunning) {
        camera.grab();
        camera.retrieve(data, raspicam::RASPICAM_FORMAT_IGNORE);
        QImage image = QImage(data, camera.getWidth(), camera.getHeight(), QImage::Format_RGB888);

        // Update the background of chart
        ui->customPlot->axisRect()->setBackground(QPixmap::fromImage(image));
        // Accelerate the current event
        QApplication::processEvents();
    }
    // Stop camera
    camera.release();
}

void MainWindow::on_btnStop_clicked()
{
    // Update the GUI buttons
    ui->btnStop->setEnabled(false);
    ui->btnStart->setEnabled(true);
    ui->btnClose->setEnabled(true);
    // Stop the camera
    dataTimer->stop();
    IsCameraRunning = false;
    // Clear the data in the colormap
    colorMap->data()->clear();
    // Clear the camera image in the background of colormap
    ui->customPlot->axisRect()->setBackground(QPixmap());
    // Update the colormap
    ui->customPlot->replot();
}

void MainWindow::on_btnClose_clicked()
{
    // Stop Camera
    camera.release();
    // Close GUI
    MainWindow::close();
}


void MainWindow::on_dialThreshold_valueChanged(int value)
{
    sound_camera->SetThresholdLevel((uint16_t) value);
}
