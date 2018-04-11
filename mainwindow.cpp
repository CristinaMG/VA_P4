#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    if(!cap->isOpened())
        cap = new VideoCapture(1);
    capture = true;
    showColorImage = false;
    winSelected = false;
    cap->set(CV_CAP_PROP_FRAME_WIDTH, 320);
    cap->set(CV_CAP_PROP_FRAME_HEIGHT, 240);
    imgS = new QImage(320,240, QImage::Format_RGB888);
    visorS = new RCDraw(320,240, imgS, ui->imageFrameS);
    imgD = new QImage(320,240, QImage::Format_RGB888);
    visorD = new RCDraw(320,240, imgD, ui->imageFrameD);

    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destGrayImage.create(240,320,CV_8UC1);
    gray2ColorImage.create(240,320,CV_8UC3);
    destGray2ColorImage.create(240,320,CV_8UC3);

    regions.create(240,320,CV_32SC1);
    //i fila, j col


    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));

    connect(ui->loadButton,SIGNAL(clicked(bool)),this,SLOT(load_image()));
    connect(ui->checkBoxBorder, SIGNAL(clicked(bool)), this, SLOT(draw_borders()));

    timer.start(60);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
    delete imgS;
    delete imgD;

}

void MainWindow::compute()
{

    regions.setTo(-1);
    regionsList.clear();

    if(capture && cap->isOpened())
    {
        *cap >> colorImage;

        cvtColor(colorImage, grayImage, CV_BGR2GRAY);
        cvtColor(colorImage, colorImage, CV_BGR2RGB);

    }

    segmentation_image();

    if(showColorImage)
    {
        memcpy(imgS->bits(), colorImage.data , 320*240*3*sizeof(uchar));
        memcpy(imgD->bits(), destColorImage.data , 320*240*3*sizeof(uchar));
    }
    else
    {
        cvtColor(grayImage,gray2ColorImage, CV_GRAY2RGB);
        cvtColor(destGrayImage,destGray2ColorImage, CV_GRAY2RGB);
        memcpy(imgS->bits(), gray2ColorImage.data , 320*240*3*sizeof(uchar));
        memcpy(imgD->bits(), destGray2ColorImage.data , 320*240*3*sizeof(uchar));

    }

    if(winSelected)
    {
        visorS->drawSquare(QPointF(imageWindow.x+imageWindow.width/2, imageWindow.y+imageWindow.height/2), imageWindow.width,imageWindow.height, Qt::green );
    }
    visorS->update();
    visorD->update();

}

void MainWindow::start_stop_capture(bool start)
{
    if(start)
    {
        ui->captureButton->setText("Stop capture");
        capture = true;
    }
    else
    {
        ui->captureButton->setText("Start capture");
        capture = false;
    }
}

void MainWindow::change_color_gray(bool color)
{
    if(color)
    {
        ui->colorButton->setText("Gray image");
        showColorImage = true;
    }
    else
    {
        ui->colorButton->setText("Color image");
        showColorImage = false;
    }
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if(w>0 && h>0)
    {
        imageWindow.x = p.x()-w/2;
        if(imageWindow.x<0)
            imageWindow.x = 0;
        imageWindow.y = p.y()-h/2;
        if(imageWindow.y<0)
            imageWindow.y = 0;
        pEnd.setX(p.x()+w/2);
        if(pEnd.x()>=320)
            pEnd.setX(319);
        pEnd.setY(p.y()+h/2);
        if(pEnd.y()>=240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x()-imageWindow.x;
        imageWindow.height = pEnd.y()-imageWindow.y;

        winSelected = true;
    }
}

void MainWindow::deselectWindow()
{
    winSelected = false;
}

void MainWindow::load_image()
{
    QString fileName = QFileDialog::getOpenFileName(this,
           tr("Open File"), "",
           tr("All Files (*)"));

    if (fileName.isEmpty())
           return;
    else {

           colorImage = imread(fileName.toStdString(), CV_LOAD_IMAGE_COLOR);
           cv::resize(colorImage, colorImage, Size(320,240));
           cvtColor(colorImage, colorImage, CV_BGR2RGB);
           cvtColor(colorImage, grayImage, CV_BGR2GRAY);

           ui->captureButton->setText("Start capture");
           capture = false;
           ui->captureButton->setChecked(false);
    }
}

void MainWindow::segmentation_image()
{
    int numberRegions = 0;

    Canny(grayImage, borders, 40, 120);

    for(int i = 0; i<grayImage.rows; i++){
        for(int j = 0; j<grayImage.cols; j++){
            if(regions.at<int>(i,j) == -1){
                create_region(Point(j,i), numberRegions);
                numberRegions++;

            }
        }

    }

    for (int var = 0; var < regions.rows; ++var) {
        for (int var2 = 0; var2 < regions.cols; ++var2) {
            destGrayImage.at<uchar>(var,var2) = regionsList.at(regions.at<int>(var,var2)).gray;
        }
    }
}

void MainWindow::create_region(Point inicial, int numberRegion){
    std::vector<Point> list;
    Point pAct;
    uint i =0;
    uchar valueGray;
    list.push_back(inicial);


    valueGray = grayImage.at<uchar>(inicial);
    while(i<list.size()){
        pAct = list[i];

        if(pAct.x >=0 && pAct.x<grayImage.cols && pAct.y>=0 && pAct.y<grayImage.rows && regions.at<int>(pAct) == -1 && borders.at<uchar>(pAct)==0){
            if(abs(grayImage.at<uchar>(pAct.y, pAct.x)-valueGray) < 50)
            {
                regions.at<int>(pAct.y, pAct.x) = numberRegion;
                list.push_back(Point(pAct.x-1, pAct.y));
                list.push_back(Point(pAct.x, pAct.y-1));
                list.push_back(Point(pAct.x+1, pAct.y));
                list.push_back(Point(pAct.x, pAct.y+1));
            }
        }
        i++;
    }

    region region;
    region.first = inicial;
    region.gray = grayImage.at<uchar>(inicial);
    region.id = numberRegion;
    region.numPoints = list.size();

    regionsList.push_back(region);

/*
    for(int i = 0; i<grayImage.rows; i++){
        for(int j = 0; j<grayImage.cols; j++){
            if(regions.at<int>(i,j) == -1){
                valueGray = grayImage.at<uchar>(i,j);
                int min = 1000, reg = -1;
                if(i>0 && min > abs(valueGray - grayImage.at<uchar>(i-1, j)) && regions.at<int>(i-1, j)!= -1){
                    min = abs(valueGray - grayImage.at<uchar>(i-1, j));
                    reg = regions.at<int>(i-1, j);
                }


                if(i<grayImage.rows-1 && min > abs(valueGray - grayImage.at<uchar>(i+1, j)) && regions.at<int>(i+1, j)!= -1){
                    min = abs(valueGray - grayImage.at<uchar>(i+1, j));
                    reg = regions.at<int>(i+1, j);
                }


                if(j>0 && min > abs(valueGray - grayImage.at<uchar>(i, j-1)) && regions.at<int>(i, j-1)!= -1){
                    min = abs(valueGray - grayImage.at<uchar>(i, j-1));
                    reg = regions.at<int>(i, j-1);
                }

                if(j<grayImage.cols-1 && min > abs(valueGray - grayImage.at<uchar>(i, j+1)) && regions.at<int>(i, j+1)!= -1){
                    min = abs(valueGray - grayImage.at<uchar>(i, j+1));
                    reg = regions.at<int>(i, j+1);
                }



                regions.at<int>(i,j) = reg;
                //qDebug()<<regionsList.size() << "reg" <<reg;
                regionsList.at(reg).numPoints++;
            }
        }
    }*/


    list.clear();
}

void MainWindow::draw_borders(){
    //find_borders();
}
