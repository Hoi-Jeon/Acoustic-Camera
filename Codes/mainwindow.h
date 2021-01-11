#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

// External libaray: QCustomPlot
#include "qcustomplot.h"
// External library: Raspicam
#include <raspicam/raspicam.h>
// Communicates with MATRIX device
#include "matrix_hal/matrixio_bus.h"
// Interfaces with microphone array
#include "matrix_hal/microphone_array.h"
// External library: sound_camera
#include "sound_camera.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnStart_clicked();
    void on_btnStop_clicked();    
    void on_btnClose_clicked();
    void realtimeDataSlot();
    void on_dialThreshold_valueChanged(int value);

private:
    Ui::MainWindow *ui;
    // Timer for data
    QTimer* dataTimer;
    // Flag for check Camera run
    bool IsCameraRunning;
    // Create Colormap
    QCPColorMap* colorMap;
    // Creat FFT map
    QCPBars* FFTbar;
    // Create camera object
    raspicam::RaspiCam camera;
    // Create MatrixIOBus object for hardware communication
    matrix_hal::MatrixIOBus bus;
    // Create MicrophoneArray object
    matrix_hal::MicrophoneArray microphone_array;
    // Create a sound camera
    SoundCamera* sound_camera;
};

#endif // MAINWINDOW_H
