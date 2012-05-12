#pragma once

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QThread>
#include <QTimer>
#include <QDebug>

#include "TheoraVideoManager.h"
#include "TheoraVideoClip.h"
#include "TheoraVideoFrame.h"

#include "global.h"

class VideoWidget: public QWidget{
    Q_OBJECT

public:
    VideoWidget(QWidget * parent = 0) : QWidget(parent)
    {
        mgr = NULL;
        clip = NULL;
        currentFrame = NULL;

        this->setAutoFillBackground(false);
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

public slots:
    void loadVideo(QString fileName)
    {
        mgr = new TheoraVideoManager();
        clip = mgr->createVideoClip(qPrintable(fileName), TH_RGB, 16);
        mgr->setDefaultNumPrecachedFrames(32);

        img = new QImage(clip->getWidth(), clip->getHeight(), QImage::Format_RGB32);
        w = clip->getWidth();
        h = clip->getHeight();

        clip->waitForCache();
        clip->pause();

        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), SLOT(animate()));
        timer->start(15);
    }

    void playVideo()
    {
        clip->play();
        started = true;
    }

    void pauseVideo()
    {
        clip->pause();
    }

    bool isPaused()
    {
        return clip->isPaused();
    }

    void setLoop(bool isLooping)
    {
        clip->setAutoRestart(isLooping);
    }

protected:

    void paintEvent (QPaintEvent *)
    {
        if(currentFrame)
        {
            unsigned char* data = currentFrame->getBuffer();
            unsigned char* img_data = img->bits();

            for(int i = 0; i < w*h*3; i += 3)
            {
                int pixelIdx = (i / 3) * 4;

                img_data[pixelIdx+0] = data[i+0]; // r
                img_data[pixelIdx+1] = data[i+1]; // g
                img_data[pixelIdx+2] = data[i+2]; // b
            }

            // To use OpenGL, just replace the following code in a QGLWidget
            QPainter painter(this);
            painter.fillRect(this->rect(), Qt::black);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            // Reshape to better fit widget dimensions
            double widgetWidth = width();
            double widgetHeight = height();
            double aspectRatio = double(h) / w;
            double newHeight = widgetWidth * aspectRatio;
            QRect videoImageRect(0,(widgetHeight - newHeight) * 0.5,widgetWidth, newHeight);

            painter.drawImage(videoImageRect,*img);
        }
    }

    void updateTime(float time_increase)
    {
        if (started)
        {
            // let's wait until the system caches up a few frames on startup
            if (clip->getNumReadyFrames() < clip->getNumPrecachedFrames()*0.5f)
                return;
            started=0;
        }
        mgr->update(time_increase);
    }

public slots:

    void animate()
    {
        /* Timing related code */
        static unsigned long time = GetTickCount();
        unsigned long t = GetTickCount();

        float diff = (t-time) / 1000.0f;
        if (diff > 0.25f) diff=0.05f; // prevent spikes (usually happen on app load)
        updateTime(diff);

        time = t;

        #ifdef _DEBUG
        // FPS stuff
        static unsigned int fps_timer = time, fps_counter = 0;
        if (t - fps_timer >= 250){
            //char title[512],debug[256]="";
            qDebug() << "FPS " << fps_counter*4;
            fps_counter=0;
            fps_timer=t;
        }
        else fps_counter++;
        #endif

        /* Refresh widget to draw frame */
        nextFrame = clip->getNextFrame();
        if(nextFrame && nextFrame != currentFrame)
        {
            currentFrame = nextFrame;
            repaint();
        }
    }

    int videoWidth() {return clip->getWidth();}
    int videoHeight() {return clip->getHeight();}

    void saveCurrentFrame(QString fileName = "snapshot.png") { img->save(fileName); }

    void seekVideo(int percent)
    {
        double time = clip->getDuration() * (double(percent) / 100.0);
        clip->seek(time);
        repaint();
    }

    void togglePlay()
    {
        if(isPaused())
            clip->play();
        else
            clip->pause();
    }

private:

    TheoraVideoManager *mgr;
    TheoraVideoClip *clip;
    TheoraVideoFrame *currentFrame, *nextFrame;

    QImage * img;
    int w;
    int h;

    bool started;
};