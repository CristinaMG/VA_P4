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
    draw_borders();

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
    uchar valueGray = 0;

    Canny(grayImage, borders, 40, 120);

    for(int i = 0; i<grayImage.rows; i++){
        for(int j = 0; j<grayImage.cols; j++){
            if(regions.at<int>(i,j) == -1){
                create_region(Point(j,i), numberRegions);
                numberRegions++;

            }
        }

    }

    int min, reg;
    for(int i = 0; i<grayImage.rows; i++){
        for(int j = 0; j<grayImage.cols; j++){
            if(regions.at<int>(i,j) == -1){

                valueGray = grayImage.at<uchar>(i,j);
                min = 1000, reg = -1;
                for(int k = i-1; k<i+2; k++){
                    for(int l = j-1; l<j+2; l++){
                        if(l>=0 && k>=0 && l<grayImage.cols && k<grayImage.rows
                                && min > abs(valueGray - grayImage.at<uchar>(k, l))
                                && regions.at<int>(k, l)!= -1
                                && (l!=j || k!=i)){
                            min = abs(valueGray - grayImage.at<uchar>(k, l));
                            reg = regions.at<int>(k, l);
                        }
                    }
                }
                regions.at<int>(i,j) = reg;
                regionsList.at(reg).numPoints++;
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
    list.push_back(inicial);
    uchar valueGray = grayImage.at<uchar>(inicial);
    float av = 0.0, avNew = 0.0, dt = 0.0, dtNew = 0.0;
    int cont = 0;

    while(i<list.size()){
        pAct = list[i];

        if(pAct.x >=0 && pAct.x<grayImage.cols && pAct.y>=0 && pAct.y<grayImage.rows && regions.at<int>(pAct) == -1 && borders.at<uchar>(pAct)==0){
            if(ui->checkBoxStatistics->isChecked()){

                avNew = (av * cont + grayImage.at<uchar>(pAct))/(cont+1);
                cont++;
                dtNew = dt + (grayImage.at<uchar>(pAct)-av)*(grayImage.at<uchar>(pAct)-avNew);

                if(sqrt(dtNew/cont) < 20.0)
                {
                    av = avNew;
                    dt = dtNew;
                    regions.at<int>(pAct.y, pAct.x) = numberRegion;
                    list.push_back(Point(pAct.x-1, pAct.y));
                    list.push_back(Point(pAct.x, pAct.y-1));
                    list.push_back(Point(pAct.x+1, pAct.y));
                    list.push_back(Point(pAct.x, pAct.y+1));
                }
            }else{
                if(abs(grayImage.at<uchar>(pAct.y, pAct.x)-valueGray) < 50)
                {
                    regions.at<int>(pAct.y, pAct.x) = numberRegion;
                    list.push_back(Point(pAct.x-1, pAct.y));
                    list.push_back(Point(pAct.x, pAct.y-1));
                    list.push_back(Point(pAct.x+1, pAct.y));
                    list.push_back(Point(pAct.x, pAct.y+1));
                }
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
    list.clear();

}

void MainWindow::draw_borders(){
    if(ui->checkBoxBorder->isChecked()){
        find_borders();
        for(uint i = 0; i<regionsList.size();i++){
            for(uint j = 0; j<regionsList.at(i).frontier.size(); j++){
                visorD->drawSquare(QPointF(regionsList.at(i).frontier.at(j).x-1,regionsList.at(i).frontier.at(j).y-1), 2,2, Qt::green );
            }
        }
    }
}

void MainWindow::find_borders(){
    bool border = false;
    for(int i = 0; i<grayImage.rows; i++){
        for(int j = 0; j<grayImage.cols; j++){
            border = false;
            if(i>0 && regions.at<int>(i-1, j) != regions.at<int>(i,j)){
                regionsList.at(regions.at<int>(i,j)).frontier.push_back(Point(j,i));
                border = true;
            }
            if(j>0 && regions.at<int>(i,j-1) != regions.at<int>(i,j) && !border){
                regionsList.at(regions.at<int>(i,j)).frontier.push_back(Point(j,i));
                border=true;
            }
            if(i<grayImage.rows-1 && regions.at<int>(i+1,j) != regions.at<int>(i,j) && !border){
                regionsList.at(regions.at<int>(i,j)).frontier.push_back(Point(j,i));
                border=true;
            }
            if(j<grayImage.cols-1 && regions.at<int>(i,j+1) != regions.at<int>(i,j) && !border){
                regionsList.at(regions.at<int>(i,j)).frontier.push_back(Point(j,i));
                border=true;
            }
        }
    }
}

float MainWindow::typical_deviation(Point p, int numberRegion){

    float av = 0.0, dt = 0.0;
    int cont = 0;
    for(int i = 0; i<regions.rows; i++){
        for(int j = 0; j<regions.cols; j++){
            if(regions.at<int>(i,j)==numberRegion){
                av = (av * cont + grayImage.at<uchar>(i,j))/(cont+1);
                cont++;
            }
        }
    }

    qDebug()<<av;
/*
    for(int i = 0; i<regions.rows; i++){
        for(int j = 0; j<regions.cols; j++){
            if(regions.at<int>(i,j)==numberRegion){
                dt += pow(grayImage.at<uchar>(i,j)-av,2);
            }
        }
    }

    dt=sqrt(dt+pow(grayImage.at<uchar>(p)-av,2)/cont);
    //qDebug()<<dt;
    return dt;*/

    return abs(av - grayImage.at<uchar>(p));
}
