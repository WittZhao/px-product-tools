﻿#include "mainwin.h"
#include "ui_mainwin.h"
#include <QVBoxLayout>
#include <QSettings>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include "access_helper.h"
#include "fuse_parser.h"
#include "param.h"
#include "setting_helper.h"
#include "qxtglobal.h"
#include "qxtglobalshortcut.h"
#include "qxtglobalshortcut_p.h"
#include "json_helper.h"
#include "md5helper.h"
#include "gps_helper.h"
#include <QtXlsx>
#include "at_parser.h"
#include "httppost.h"
#include<QObject>
#include<QNetworkReply>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include "globle.h"
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <aes128.h>
#include <stdint.h>

#define AT_WAIT_TIME_DEFAULT 500
#define AT_WAIT_TIME_CONNECT 100*40
#define AT_WAIT_TIME_SEND 100*30
#define AT_WAIT_TIME_DELAY 100*5

#define AT_WAIT_TIME_DELAY_2s 100*2
#define AT_WAIT_TIME_DELAY_3s 100*3
#define AT_WAIT_TIME_DELAY_4s 100*4
#define AT_WAIT_TIME_DELAY_5s 100*5
#define AT_WAIT_TIME_DELAY_8s 100*8
#define AT_WAIT_TIME_DELAY_60s  100*60
#define SECONDS *100
#define OTA_SLEEP   120


#define AT_WAIT_TIME_QIDEACT 100*60
#define AT_WAIT_TIME_QIACT 100*60



#define excel_ccid_loc 1
#define excel_imsi_loc 2
#define excel_ceid_loc 3
#define excel_cardid_loc 4
#define excel_newccid_loc 5
#define excel_imei_loc 6
#define excel_newnum_status_loc 7
#define excel_ota_status_loc 8
#define excel_check_imsi    9
#define excel_check_cardnum    10


mainWin::mainWin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainWin)
{
    ui->setupUi(this);
    //让窗口居中显示
    //utilMan::windowCentered(this);
    //初始化窗口
    this->initWindow();
}

mainWin::~mainWin()
{
    setting_helper::test_config_save(cmd_test_config);
    delete ui;
}

void mainWin::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

/**
 * @brief mainWin::closeEvent
 * @param event
 */
void mainWin::closeEvent(QCloseEvent *event)
{
    if(saveDataFlag)
    {
        file_save->close();
        //delete text_save;
    }
    pmainlog->close();
    event->accept();
}

void mainWin::keyPressEvent(QKeyEvent *e)
{
    qDebug()<<QString("key:%1,%2").arg(e->key()).arg(e->text());
}

void mainWin::moveEvent(QMoveEvent *e)
{
    pmainlog->move(this->pos().x()-pmainlog->width()-16,this->pos().y());
    pmainlog->activateWindow();
    pmainlog->raise();
    pmainlog->showNormal();
}

bool mainWin::eventFilter(QObject *obj, QEvent *e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *event = static_cast<QKeyEvent*>(e);
        // if (event->key() == Qt::Key_Return && (event->modifiers() & Qt::ControlModifier))
        if (event->key() == Qt::Key_Return)
        {
            //on_button_com_write_clicked();
            return true;
        }
        if (event->key() == Qt::Key_Down)
        {
            //show_log("down...");
            return true;
        }
    }
    return false;
}



void mainWin::id_cnt_update()
{
#if 0
    access_helper::id_re_count();
 #endif
    ui->lb_id_used->setText(QString("imei 写入:%1").arg(g_db.imei_records_all));
    ui->lb_check_ota_result->setText(QString("ccid 读取:%1").arg(g_db.ccid_records_all));
}
void mainWin::initWindow()
{
    //json_helper json_h;
    // QString s=json_h.json_make();
    // json_h.json_parse(s);
    json_helper *pjson=new json_helper();
    QString s=pjson->json_make();
    pjson->json_parse(s);
    mode=work_mode_param;
    // QxtGlobalShortcut* shortcut = new QxtGlobalShortcut(QKeySequence("Enter"), this);
    // connect(shortcut, SIGNAL(activated()), this, SLOT(process_key_fuse_start()));
    // QxtGlobalShortcut* shortcut2 = new QxtGlobalShortcut(QKeySequence(Qt::Key_Space), this);
    // connect(shortcut2, SIGNAL(activated()), this, SLOT(process_key_fuse_stop()));

    param_t  r=setting_helper::param_read();
    test_cmd_block.state=at_state_idle;
    gps_helper::gps_param.num=r.gps_test_sat_num.toInt();
    gps_helper::gps_param.sni=r.gps_test_min_nic.toInt();
    gps_helper::gps_param.time=r.gps_test_max_time.toInt();
    gps_helper::gps_param.time_agps=r.gps_test_max_time_agps.toInt();
    setting_helper::app_config_get(config);

    ui->cbprintcom->setChecked(config.is_com_print);
    ui->send_16_CheckBox->setChecked(config.is_com_send_hex_print);
    ui->recv_16_CheckBox->setChecked(config.is_com_recv_hex_print);
    ui->CheckBoxIsBoot->setChecked(config.is_boot_upgrade);
    ui->cb_config_ccid_read->setChecked(config.is_imei_write_then_ccid_read);
    ui->cb_config_imei_repeated_check->setChecked(config.is_imei_repeated_check);
    ui->cb_config_imei_is_infront->setChecked(config.is_imei_infront);
    ui->cb_config_gps_autoopen->setChecked(config.is_gps_autoopen);
    ui->cb_config_gps_checksim->setChecked(config.is_gps_check_sim);
    ui->cb_com_cr_show->setChecked(config.is_com_cr_show);

    testconfiginit();
    //plot_init();
    bar_init();
    com_init();

//     //QString fname=QDateTime::currentDateTime().toString("yyyy-mm-dd HH:mm:ss.zzz");
//     QDateTime dt=QDateTime::currentDateTime();
//     QString fname =dt.toString("yyyy-mm");
//    // QString t=    QDateTime::currentDateTime().toString("yyyy-mm-dd HH:mm:ss.zzz");
//    // show_log(fname);
////    file_save = new QFile();
////    if(file_save!=NULL&&file_save->open(QFile::WriteOnly|QFile::Append))
////    {
////        show_log("log creat ok");;
////    }
////    else
////    {
////        show_log("log creat fail");
//    }
    comthread=new com_thread(QString("testname"));
    connect(comthread,SIGNAL(processProgress(int)),this,SLOT(processProgress(int)));
    connect(comthread,SIGNAL(processFinished(QString)),this,SLOT(processFinished(QString)));
    //comthread->start();


    at_net_cmd_block.timer = new QTimer(this);
    //at_gps_writer.timer->setInterval(9);
    at_net_cmd_block.timer->start(10);
    connect(at_net_cmd_block.timer,SIGNAL(timeout()),this,SLOT(on_at_timer()));

    //显示时间
    clock=new QTimer(this);
    clock->start(1000);
    connect(clock,SIGNAL(timeout()),this,SLOT(on_time_update()));
    //显示日期
    QDate dateNow=QDate::currentDate();
    ui->date_Label->setText(QString("日期:%1").arg(dateNow.toString("yy年MM月dd日 dddd")));
    //在没有打开串口之前其他框内的一切都不可用
    this->changeEnable(false);

    //  progressBar = new QProgressBar(ui->sending_TextEdit);
    // progressBar->setGeometry(0,0,0,0);//这里起到一个影藏的效果
    // progressBar->setValue(0);
    this->setWindowTitle("---AT生产---");

    connect(ui->cbprintcom,SIGNAL(clicked(bool)),this,SLOT(on_print_com_clicked(bool)));
    reset_show=new DialogReset();
    pmainlog= new mainlog();
    pmainlog->show();
    pmainlog->move(10,10);
    this->move(10+pmainlog->width()+20,10);
    p_gps_setting=new gps_setting();
    connect(pmainlog,SIGNAL(posChanged()),this,SLOT(process_show_slot()));
    // pmainlog->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    //ui->recv_GroupBox->setParent(NULL);
    //ui->pte_com_write->installEventFilter(this);
    md5Helper md5;
    // QString sign= md5.encryption(deviceName,productKey,deviceSecret,productSecret);
    //QString sign= md5.encryption(param_config.deviceName,param_config.product_key,param_config.deviceSecret,param_config.product_secret);

    //client=new httppost();
    // connect(client,SIGNAL(host_get(QString)),this,SLOT(on_mqtt_host_get(QString)));
    // client->post(param_config.deviceName,param_config.product_key,sign);
    param_edit_enable(false);
    //---sqlite test---
    g_db.createConnection();  //创建连接
    id_cnt_update();
    g_network_manager = new QNetworkAccessManager();
    g_http_test_on=0;
    connect(g_network_manager,SIGNAL(finished(QNetworkReply*)), this, SLOT(reply_http_get(QNetworkReply*)));
    //d.createTable();
   // d.insert();
   // d.queryAll();          //已经完成过createTable(), insert(), 现在进行查询
    //----------

}
void mainWin::ota_post_data_display(QString data)
{
    ui->lb_ota_post_data->setText(data);
}
void mainWin::ota_post_result_display(QString data)
{
    ui->lb_ota_post_result->setText(data);
}

void mainWin::testconfiginit()
{
    QGridLayout * mainLayout= new QGridLayout(ui->gb_test);

    setting_helper::test_config_get(cmd_test_config);

    int row;
    row=0;
    for(int i=0;i<cmd_test_config.num_all;i++)
    {
        //codec = QTextCodec::codecForName("GBK");//在windows下所以用的GBK
        cmd_test_config.cmds[i].button=new QPushButton(cmd_test_config.cmds[i].caption,ui->gb_test);
        cmd_test_config.cmds[i].textedit=new QTextEdit(cmd_test_config.cmds[i].cmd,ui->gb_test);
        //QRect rect =cmd_test_config.cmds[i].button->geometry();
        cmd_test_config.cmds[i].button->setFixedHeight(cmd_test_config.buttonheight);
        cmd_test_config.cmds[i].textedit->setFixedHeight(cmd_test_config.buttonheight);

        connect(cmd_test_config.cmds[i].button,SIGNAL(clicked()),this,SLOT(process_key_test()));

        row++;
        mainLayout->addWidget(cmd_test_config.cmds[i].textedit, row,0);
        mainLayout->addWidget(cmd_test_config.cmds[i].button, row,1);
    }
    ui->gb_test->setLayout(mainLayout);
}

void mainWin::com_init()
{
    recvCount = 0;
    sendCount = 0;

    saveDataFlag = false;
    oldSize = 0;
    write_step=write_step_idle;
    read_step=read_step_idle;
    is_bin_writing=false;
    at_net_cmd_block.binsize=0;

    QStringList comList;        //串口号
    QStringList baudList;       //波特率
    QStringList parityList;     //校验位
    QStringList dataBitsList;   //数据位
    QStringList stopBitsList;   //停止位

    codec = QTextCodec::codecForName("GBK");//在windows下所以用的GBK
    //获取串口
#ifdef Q_OS_WIN//如果是windows系统
    QString path="HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\SERIALCOMM";
    QSettings *settings=new QSettings(path,QSettings::NativeFormat);
    QStringList key=settings->allKeys();
    int kk = key.size();
    int i;
    comList.clear();
    for(i=0;i<kk;i++)
    {
        comList << getPortName_win(i,"value");
    }
    comList.sort();
#else//如果是unix系统
    getPortName(comList);
#endif
    //波特率列表
    baudList<<"50"<<"75"<<"100"<<"134"<<"150"<<"200"<<"300"
           <<"600"<<"1200"<<"1800"<<"2400"<<"4800"<<"9600"
          <<"14400"<<"19200"<<"38400"<<"56000"<<"57600"
         <<"76800"<<"115200"<<"128000"<<"921600";

    //校验位列表
    parityList<<"无"<<"奇"<<"偶";
#ifdef Q_OS_WIN//如果是windows系统
    parityList<<"标志";
#endif
    parityList<<"空格";

    //数据位列表
    dataBitsList<<"5"<<"6"<<"7"<<"8";

    //停止位列表
    stopBitsList<<"1";
#ifdef Q_OS_WIN//如果是windows系统
    stopBitsList<<"1.5";
#endif
    stopBitsList<<"2";


    ui->portName_cBox->addItems(comList);
    ui->baud_cBox->addItems(baudList);
    ui->baud_cBox->setCurrentIndex(19);
    ui->dataBit_cBox->addItems(dataBitsList);
    ui->dataBit_cBox->setCurrentIndex(3);
    ui->parity_cBox->addItems(parityList);
    ui->stopBit_cBox->addItems(stopBitsList);

    //读取数据(采用定时器读取数据，不采用事件，方便移植到linux)
    readTimer=new QTimer(this);
    //10毫秒采集一次
    readTimer->setInterval(2);
    connect(readTimer,SIGNAL(timeout()),this,SLOT(com_read()));
}

void mainWin::show_log(QString s)
{
    //ui->log_textEdit->append(QString("【%1】:%2").arg(QTime::currentTime().toString("HH:mm:ss.zzz")).arg(s));
    //ui->log_textEdit->moveCursor(QTextCursor::End);
    pmainlog->show_log(s,false,QColor("black"),config.is_com_cr_show);
}
void mainWin::show_log(QString s,QColor c, bool showtime)
{
    pmainlog->show_log(s,showtime,c,config.is_com_cr_show);
}

void mainWin::show_gps_result(QString s, QString color)
{
    ui->lb_gps_result->setText(QString("<font color='#%1' ><b><p align='center'>%2</p></b></font>").arg(color).arg(s));
}
void mainWin::show_agps_result(QString s, QString color)
{
    ui->lb_agps_result->setText(QString("<font color='#%1' ><b><p align='center'>%2</p></b></font>").arg(color).arg(s));
}
void mainWin::show_gsensor_result(QString s, QString color)
{
    ui->lb_gsensor_result->setText(QString("<font color='#%1' ><b><p align='center'>%2</p></b></font>").arg(color).arg(s));
}

void mainWin::on_time_update()
{
    static int http_tick;
    http_tick++;
    int http_inter=ui->sb_http_inter->value();
    if(http_inter==0)
        http_inter=5;
    if(g_http_test_on)
    {
        if(http_tick>=http_inter)
        {
            http_tick=0;
            QString url=ui->te_http_get_url->toPlainText();
            QUrl turl(url);
            ui->te_http_get_result->clear();
            g_network_manager->get(QNetworkRequest(turl));
            g_http_test_cnt++;
            show_log(QString("http get %1").arg(g_http_test_cnt),QColor("black"),true);
        }
    }
    QTime timeNow=QTime::currentTime();
    ui->showTime_Label->setText(QString("时间:%1").arg(timeNow.toString()));
    ui->lb_gps_cnt->setText(QString().sprintf("gps tx:%d, rx:%d\r\nnet tx:%d,rx:%d\r\nerror:%d\r\n",
                                              at_gps_cmd_block.gps_request_cnt,
                                              at_gps_cmd_block.gps_ack_cnt,
                                              at_net_cmd_block.net_send_cnt,
                                              at_net_cmd_block.net_send_ok_cnt,
                                              at_net_cmd_block.net_error_cnt));
    if(at_gps_cmd_block.is_factory_gps_snr_test)
    {
        if(at_gps_cmd_block.gps_test_life<gps_helper::gps_param.time)
        {
            at_gps_cmd_block.gps_test_life++;
            int percent=at_gps_cmd_block.gps_test_life*100/gps_helper::gps_param.time;
            ui->progressBar_gps_test->setValue(percent);
        }
        else
        {
            ui->pushButton_gps_stop->click();
            show_gps_result("TIMEOUT","ff0000");
            at_gps_cmd_block.is_factory_gps_snr_test=0;
            ui->progressBar_gps_test->setValue(0);
            save_gps_test_result("TIMEOUT", test_cmd_block.ota_imeir_text);
        }
    }
    if(at_gps_cmd_block.is_factory_agps_test)
    {
        if(at_gps_cmd_block.agps_test_life<gps_helper::gps_param.time_agps)
        {
            at_gps_cmd_block.agps_test_life++;
            int percent=at_gps_cmd_block.agps_test_life*100/gps_helper::gps_param.time_agps;
            ui->progressBar_agps_test->setValue(percent);
        }
        else
        {
            ui->pushButton_agps_stop->click();
            show_agps_result("TIMEOUT","ff0000");
        }
    }
}

/**
  *在初始化的时候，将除了串口参数设置区外都设置为不可用
  *在打开串口后，改变这些框为可用
  */
void mainWin::changeEnable(bool flag)
{
    int i;
    //ui->recv_GroupBox->setEnabled(flag);
  //  ui->sending_GroupBox->setEnabled(flag);
    //ui->recvSet_GroupBox->setEnabled(flag);
    ui->dataSet_GroupBox->setEnabled(flag);

    ui->ButtonBinWriteStart->setEnabled(flag);
    // ui->ButtonBinWriteStop->setEnabled(flag);

    //ui->pte_com_write->setEnabled(flag);
    //ui->button_com_write->setEnabled(flag);

    ui->portName_cBox->setEnabled(!flag);
    ui->baud_cBox->setEnabled(!flag);
    ui->parity_cBox->setEnabled(!flag);
    ui->dataBit_cBox->setEnabled(!flag);
    ui->stopBit_cBox->setEnabled(!flag);

    is_bin_writing=!flag;
    for(i=0;i<cmd_test_config.num_all;i++)
    {
        if(cmd_test_config.cmds[i].button!=NULL)
        {
            cmd_test_config.cmds[i].button->setEnabled(flag);
        }
    }
}

/**
  *打开窜口按钮槽函数
  */
void mainWin::on_open_Button_clicked()
{
    if (ui->open_Button->text()=="打开串口")
    {
        QString portName=ui->portName_cBox->currentText();

#ifdef Q_OS_WIN//如果是windows系统
        myCom = new QextSerialPort(portName);
#else
        myCom = new QextSerialPort("/dev/" + portName);
#endif

        if (myCom->open(QIODevice::ReadWrite))
        {
            //清空缓冲区
            myCom->flush();
            //设置波特率
            myCom->setBaudRate((BaudRateType)ui->baud_cBox->currentText().toInt());
            //设置数据位
            myCom->setDataBits((DataBitsType)ui->dataBit_cBox->currentText().toInt());
            //设置校验位
            myCom->setParity((ParityType)ui->parity_cBox->currentIndex());
            //设置停止位
            myCom->setStopBits((StopBitsType)ui->stopBit_cBox->currentIndex());
            myCom->setFlowControl(FLOW_OFF);
            myCom->setTimeout(10);//定时读取数据到缓冲区

            this->changeEnable(true);
            ui->open_Button->setText("关闭串口");
            ui->open_Button->setIconSize(QSize(22,22));
            ui->open_Button->setIcon(QIcon(":/images/images/stop.png"));
            this->setWindowTitle(portName + "已打开，波特率：" + ui->baud_cBox->currentText());
            this->readTimer->start();
            //  ui->sendOne_CheckBox->setChecked(true);//将发送单个字符串勾选

            //sendingData.clear();
            com_is_open=1;
        }
        else
        {
            this->setWindowTitle("该端口被占用，无法打开！");
        }
    }
    else
    {
        com_is_open=0;
        myCom->close();
        this->changeEnable(false);
        ui->open_Button->setText("打开串口");
        ui->open_Button->setIcon(QIcon(":/images/images/start.png"));
        this->setWindowTitle("---烧写软件---");
        this->readTimer->stop();
        if(saveDataFlag)
        {
            file_save->close();
            saveDataFlag = false;
        }
        //应阿莫论坛网友的建议，增加关闭串口时重新扫描可用串口
        QStringList comList;
#ifdef Q_OS_WIN//如果是windows系统
        QString path="HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\SERIALCOMM";
        QSettings *settings=new QSettings(path,QSettings::NativeFormat);
        QStringList key=settings->allKeys();
        int kk = key.size();
        int i;
        comList.clear();
        for(i=0;i<kk;i++)
        {
            comList << getPortName_win(i,"value");
        }
        comList.sort();
#else//如果是unix系统
        getPortName(comList);
#endif
        ui->portName_cBox->clear();
        ui->portName_cBox->addItems(comList);
    }
}

/**
  *在unix系统下查找当前可用的串口
  */
void mainWin::getPortName(QStringList &comList)
{
    //调用系统命令将可用串口号的信息保存到/opt/port.txt文件中
    system("dmesg |grep tty > /opt/port.txt");
    QFile portName("/opt/port.txt");
    if(portName.open(QIODevice::ReadOnly))
    {
        QTextStream text_stream(&portName);
        QString temPort;
        //查找串口和USB转串口
        const QString com = "ttyS";
        const QString usbcom = "ttyUSB";
        int comLocation = -1;
        int usbcomLocation = -1;
        while(!text_stream.atEnd())
        {
            temPort = text_stream.readLine();
            //获取包含查找的字符串的具体位子
            comLocation = temPort.indexOf(com);
            usbcomLocation = temPort.indexOf(usbcom);
            if(comLocation >= 0)
            {
                //将截取的串口号放到comList中
                comList << temPort.mid(comLocation,5);
            }
            else if(usbcomLocation >= 0)
            {
                //将截取的USB转串口放到comList中
                comList << temPort.mid(usbcomLocation,7);
            }
        }
        //这里现将找到的所有相关信息排序，方便删除重复的
        comList.sort();
        for(int i = 1;i < comList.size();i ++)
        {
            if(comList[i] == comList[i - 1])
            {
                comList.removeAt(i);
            }
        }
    }
    //查找完毕，把生成的文件删除掉
    system("rm -f /opt/port.txt");
}

/**
  *windows下获取串口号，通过读取注册表的方式
  */
QString mainWin::getPortName_win(int index,QString keyorvalue)
{
    QString commresult="";
    QString strkey="HARDWARE\\DEVICEMAP\\SERIALCOMM";//子键路径
    int a=strkey.toWCharArray(subkey);
    subkey[a]=L'\0';
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,subkey,0,KEY_READ|KEY_QUERY_VALUE,&hKey)!=0)
    {
        QString error="Cannot open regedit!";
    }

    QString keymessage="";//键名
    QString message="";
    QString valuemessage="";//键值
    indexnum=index;//要读取键值的索引号

    keysize=sizeof(keyName);
    valuesize=sizeof(keyValue);

    if(::RegEnumValue(hKey,indexnum,keyName,&keysize,0,&type,(BYTE*)keyValue,&valuesize)==0)
    {
        //读取键名
        for(DWORD i=0;i<keysize;i++)
        {
            message=QString::fromStdWString(keyName);
            keymessage.append(message);
        }
        //读取键值
        for(DWORD j=0;j<valuesize;j++)
        {
            if(keyValue[j]!=0x00)
            {
                valuemessage.append(keyValue[j]);
            }
        }

        if(keyorvalue=="key")
        {
            commresult=keymessage;
        }
        if(keyorvalue=="value")
        {
            commresult=valuemessage;
        }
    }
    else
    {
        commresult="nokey";
    }
    ::RegCloseKey(hKey);//关闭注册表
    return commresult;
}

void mainWin::com_at_process(QByteArray combuf)
{
    QString comstr=codec->toUnicode(combuf);

    switch(test_cmd_block.state)
    {
    case at_state_idle:

        break;
    //imei
    case at_state_imei_read_wait:
       {
       QString imei=at_parser::imei_get(comstr);

       if(imei!=NULL){
           if(1 == test_cmd_block.ota_imei_check)
           {
               test_cmd_block.ota_imeir_text = imei;
               test_cmd_block.ota_imeir_text.remove(" ");
               test_cmd_block.ota_imeir_text.remove("\r");
               test_cmd_block.ota_imeir_text.remove("\n");
               if(test_cmd_block.ota_imeir_text == test_cmd_block.imeiw_imei_text){
                   //zwj IMEI一至，开始检查OTA
                   if(1 == test_cmd_block.ota_check){
                       test_cmd_block.ota_enable = 1;
                   }else if(2 == test_cmd_block.ota_check){
                       test_cmd_block.check_enable = 1;
                   }
                   test_cmd_block.ota_imei_write = 1;
                   test_cmd_block.state=at_state_ccid_read_send;
                   test_cmd_block.send_cnt=0;
                   test_cmd_block.test_enable=1;

               }else{
                   ui->lb_check_ota_result->setText("IMEI不相同，请确认");
                   ota_stop();
                   test_cmd_block.state=at_state_idle;
               }
               test_cmd_block.ota_imei_check = 0;
               break;
           }else if(1 == test_cmd_block.gps_test_imei_get){
               test_cmd_block.ota_imeir_text = imei;
               test_cmd_block.ota_imeir_text.remove(" ");
               test_cmd_block.ota_imeir_text.remove("\r");
               test_cmd_block.ota_imeir_text.remove("\n");
               test_cmd_block.gps_test_imei_get = 0;
               at_gps_cmd_block.state=at_net_state_cpin_send;
           }else if(1 == test_cmd_block.qxwz_account_set){
                test_cmd_block.qxwz_account_imei = imei;
                test_cmd_block.qxwz_account_imei .remove(" ");
                test_cmd_block.qxwz_account_imei .remove("\r");
                test_cmd_block.qxwz_account_imei .remove("\n");
                qxwz_account_set(test_cmd_block.qxwz_account_imei);
                test_cmd_block.state=at_state_qxwz_account;
                break;
           }else{
               ui->lb_imei_show->setText(imei);
               test_cmd_block.state=at_state_idle;
           }
       }
       else{
           ui->lb_imei_show->setText("xxxxx");
            qDebug()<<QString("imei read wait:")<<comstr<<"get imei:"<<imei;
//            test_cmd_block.state=at_state_idle;
       }
       }

       break;
     case at_state_imei_write_wait:
        if(comstr.contains("OK"))
        {
            if(config.is_imei_write_then_ccid_read)
            {
                test_cmd_block.state=at_state_imeiw_ccid_read_wait;
                this->com_write(QString("AT+CCID\r\n"));
                ui->lb_imei_write_status->setText("读取ccid...");
            }
            else
            {
              bool ret= g_db.imei_insert(test_cmd_block.imeiw_imei_text,test_cmd_block.imeiw_sn_text,test_cmd_block.imeiw_input_text,NULL);
              if(ret)
              {
                  ui->lb_imei_write_status->setText("写入成功");
                  id_cnt_update();
                  test_cmd_block.state=at_state_idle;
                  ui->te_imei_write->clear();
              }
              else
              {
                  ui->lb_imei_write_status->setText("数据库写入失败");
              }
            }
        }
        else
        {
            ;
        }
       break;
     case at_state_imeiw_ccid_read_wait:
        {
            QString ccid=at_parser::ccid_get(comstr);
            if(ccid!=NULL)
            {
                bool ret= g_db.imei_insert(test_cmd_block.imeiw_imei_text,test_cmd_block.imeiw_sn_text,test_cmd_block.imeiw_input_text,ccid);
                if(ret)
                {
                    ui->lb_imei_write_status->setText("写入成功");
                    id_cnt_update();
                    test_cmd_block.state=at_state_idle;
                    ui->te_imei_write->clear();
                }
                else
                {
                    ui->lb_imei_write_status->setText("数据库写入失败");
                }
             }

        }
        break;
     case at_state_ccid_read_wait:
        {
            QString ccid=at_parser::ccid_get(comstr);
            if(ccid!=NULL)
            {
              ccid=ccid.trimmed();
//              if(ccid.length() == 20)
              {
                  qDebug()<<QString("ccid read wait:")<<comstr<<"get ccid "<<ccid.length()<<"bytes :"<<ccid;
                  test_cmd_block.state=at_state_ccid_read_ack;
                  test_cmd_block.ccid_text=ccid;
              }
//              else
//              {
//                  test_cmd_block.state = at_state_ccid_read_send;
//              }
            }
            else
            {

            }
        }
        break;
    case at_state_imsi_read_wait:
       {
           QString imsi=at_parser::imsi_get(comstr);
           if(imsi!=NULL)
           {
             imsi=imsi.trimmed();
             if(imsi.length() == 15)
             {
                 qDebug()<<QString("imsi read wait:")<<comstr<<"get imsi "<<imsi.length()<<"bytes :"<<imsi;
                 test_cmd_block.state=at_state_imsi_read_ack;
                 test_cmd_block.imsi_text=imsi;
             }
           }
           else
           {

           }
       }
       break;
     case at_state_ceid_read_wait:
       {
           QString ceid=at_parser::ceid_get(comstr);
           if(ceid!=NULL)
           {
             ceid=ceid.trimmed();
             if(ceid.length() == 20)
             {
                 qDebug()<<QString("ceid read wait:")<<comstr<<"get ceid "<<ceid.length()<<"bytes :"<<ceid;
                 test_cmd_block.state=at_state_ceid_read_ack;
                 test_cmd_block.ceid_text=ceid;
             }
           }
           else
           {

           }
       }
       break;
     case at_state_gsensor_test_wait:
        comstr=comstr.toUpper();
        if(comstr.contains("OK"))
        {
            test_cmd_block.state=at_state_idle;
            show_gsensor_result("OK","00ff00");
        }
        if(comstr.contains("ERROR"))
        {
            test_cmd_block.state=at_state_idle;
            show_gsensor_result("ERROR","ff0000");
        }

        break;
     case at_state_ota_sleep:
        comstr=comstr.toUpper();
        if(comstr.contains("STNN"))
        {
            static char cnt = 0;
            if(++cnt >= 1)
            {
                test_cmd_block.wait_time_cnt = OTA_SLEEP SECONDS;
                cnt = 0;
            }
        }
        break;
    case at_state_qxwz_account:
        comstr=comstr.toUpper();
        if(comstr.contains("ACCOUNT OK")){       
            QString showlog = "设置成功" ;
            ui->lb_account_result->setText(showlog);
            ui->lb_account_result->setStyleSheet("background-color:green");
            save_account_set_result(showlog, test_cmd_block.qxwz_account_imei);
            test_cmd_block.qxwz_account_set = 0;
            test_cmd_block.state=at_state_idle;
        }
        break;
     default:
         qDebug()<<QString("unkown at state:")<<test_cmd_block.state;
        break;


    }

    if(at_gps_cmd_block.gps_loop_enable)
    {
        switch (at_gps_cmd_block.state) {
        case at_net_state_cpin_wait:
            if(comstr.contains("READY"))
            {
               at_gps_cmd_block.state=at_net_state_cpin_ack;
            }
            break;
        case at_gps_state_read_wait:
            gps_helper::gps_Pro(combuf.data(),combuf.size());
            if(gps_helper::gps_parse_ok)
            {
                at_gps_cmd_block.state=at_gps_state_read_ack;
                qDebug()<<QString("gps pasr ok");
            }
            else
            {
                if(comstr.contains("ERROR"))
                 {
                    if(comstr.contains("600")||comstr.contains("OFF"))
                        at_gps_cmd_block.state=at_gps_state_error_poweroff;
                    else
                        at_gps_cmd_block.state=at_gps_state_error_other;
                 }

            }
            show_gps_info();
            qDebug()<<QString("com get:gps read wait");
            break;
         case at_gps_state_open_wait:
             if(comstr.contains("OK"))
                 at_gps_cmd_block.state=at_gps_state_open_ack;
             if(comstr.contains("ERROR"))
                 at_gps_cmd_block.state=at_gps_state_error_other;
              qDebug()<<QString("comget:gps open wait");
            break;
         case at_gps_state_agps_wait:
         {
            int pos=comstr.indexOf(QRegExp("QAGPS[ \t\n\x0B\f\r]{1,}OK"));
            if(pos>0)
            {
                at_gps_cmd_block.state=at_gps_state_agps_ack;
                qDebug()<<QString("AGPS OK");
                //end
                ui->pushButton_agps_stop->click();
                at_gps_cmd_block.is_factory_agps_test=0;
                ui->progressBar_agps_test->setValue(0);
                show_agps_result("PASS","00ff00");
            }
            if(comstr.contains("ERROR"))
            {
                at_gps_cmd_block.state=at_gps_state_error_other;
                qDebug()<<QString("AGPS  ERROR");
                //-----------
                ui->pushButton_agps_stop->click();
                show_agps_result("ERROR","00ff00");
            }
         }
            break;
        default:
            qDebug()<<QString("comget:gps unkown:")<<at_gps_cmd_block.state;
            break;
        }
    }
    else
    {   //平时也显示gps信息
        gps_helper::gps_Pro(combuf.data(),combuf.size());
        if(gps_helper::gps_parse_ok)
        {
            show_gps_info();
        }
    }

    if(at_net_cmd_block.net_loop_enable)
    {
        switch (at_net_cmd_block.state) {

        case at_net_state_cpin_wait:
            if(comstr.contains("READY"))
                at_net_cmd_block.state=at_net_state_cpin_ack;
            break;

        case at_net_state_cgatt_wait:
            if(comstr.contains("1")&&comstr.contains("CGATT"))
                at_net_cmd_block.state=at_net_state_cgatt_ack;
            break;

        case at_net_state_qihead_wait:
            if(comstr.contains("OK"))
                at_net_cmd_block.state=at_net_state_qihead_ack;
            break;
        case at_net_state_qiopen_wait:
            if(comstr.contains("CONNECT OK"))
                at_net_cmd_block.state=at_net_state_qiopen_ack;
            if(comstr.contains("ERROR"))
                at_net_cmd_block.state=at_net_state_error;
            break;
        case at_net_state_prompt_wait:
            if(comstr.contains(">"))
                at_net_cmd_block.state=at_net_state_prompt_ack;
            break;
        case at_net_state_qidata_wait:
            if(comstr.contains("SEND OK"))
                at_net_cmd_block.state=at_net_state_qidata_ack;
            if(comstr.contains("ERROR"))
                at_net_cmd_block.state=at_net_state_error;
            break;
        case at_net_state_qideact_wait:
            if(comstr.contains("OK"))
                at_net_cmd_block.state=at_net_state_qideact_ack;
            break;
        case at_net_state_qiact_wait:
            if(comstr.contains("OK"))
                at_net_cmd_block.state=at_net_state_qiact_ack;
            break;
        default:
            break;
        }
    }
}


void mainWin::BinBufPacket(QByteArray& buf)
{
    int len=buf.size();
    int i;
    byte cc;
    byte l=at_net_cmd_block.packet_cnt&0xff;
    byte h=(at_net_cmd_block.packet_cnt>>8)&0xff;
    at_net_cmd_block.sendbuf.resize(len+9);
    at_net_cmd_block.sendbuf[0]=0x00;
    at_net_cmd_block.sendbuf[1]=0xff;
    at_net_cmd_block.sendbuf[2]=0xaa;
    at_net_cmd_block.sendbuf[3]=len+4;
    at_net_cmd_block.sendbuf[4]=0x18;
    at_net_cmd_block.sendbuf[5]=h;
    at_net_cmd_block.sendbuf[6]=l;
    for(i=0;i<len;i++)
    {
        at_net_cmd_block.sendbuf[7+i]=buf[i];
    }
    cc=at_net_cmd_block.sendbuf[3];
    for(i=0;i<len+3;i++)
    {
        cc=at_net_cmd_block.sendbuf[4+i]^cc;
    }
    at_net_cmd_block.sendbuf[6+len+1]=cc;
    at_net_cmd_block.sendbuf[6+len+2]=0xcc;
    at_net_cmd_block.packet_cnt++;
}
void mainWin::show_com_rx(QByteArray combuf)
{
    QString com_temp_string=codec->toUnicode(combuf);
    if(config.is_com_print)
    {
        //if(mode==work_mode_uart)
        //   show_log(QString("com read %1 bytes:").arg(combuf.size()),QColor("black"),true);
        show_log(com_temp_string,QColor("blue"),false);
    }
    if(config.is_com_recv_hex_print)
    {
        show_log(QString("com read:%1 bytes:").arg(combuf.size()),QColor("black"),true);
        show_log(utilMan::toHexStr(combuf),QColor("blue"),false);
    }
}
//zwj  保存GPS测试结果
void mainWin::save_gps_test_result(QString result, QString imei)
{
    QString patch = qApp->applicationDirPath();
    patch.append("/gps_test.txt");
    QFile file(patch);
    if(!file.open(QIODevice::Append|QIODevice::Text))
    {
        QMessageBox::critical(NULL,"提示","无法创建文件");
        return;
    }
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyy/MM/dd hh:mm:ss");

    QTextStream txtOutput(&file);
    txtOutput << current_date + "***" + imei + "***" + result << endl;
    file.close();
}

void mainWin::show_gps_info()
{
    int i;
    if(gps_helper::gps_parse_ok)
    {
        QTime time=QTime(gps_helper::rmc.time_hour,gps_helper::rmc.time_min,gps_helper::rmc.time_sec,0);
        QDate date=QDate(gps_helper::rmc.date_year,gps_helper::rmc.date_month,gps_helper::rmc.date_day);
        QDateTime datetime=QDateTime(date,time).addSecs(3600*8);

        ui->te_gps_rmc->clear();
        ui->te_gps_rmc->append(QString().sprintf("定位状态：%c,定位模式:%c",gps_helper::rmc.st,gps_helper::rmc.Mode));
       // ui->te_gps_rmc->append(QString().sprintf("时间：%06x %06x",gps_helper::rmc.date_bcd,gps_helper::rmc.time_bcd));
        ui->te_gps_rmc->append(QString("GNSS时间:%1").arg(datetime.toString("yyyy-MM-dd hh:mm:ss")));
        ui->te_gps_rmc->append(QString().sprintf("定位:%f,%f",gps_helper::rmc.latitude/1000000.0,gps_helper::rmc.longitude/1000000.0));
        ui->te_gps_rmc->append(QString().sprintf("速度:%fkm/h,%d",gps_helper::rmc.speed*185/100000.0,gps_helper::rmc.speed));
        ui->te_gps_rmc->append(QString().sprintf("cog:%d",gps_helper::rmc.cog));
        ui->te_gps_rmc->append(QString().sprintf("方向:%c，%c",gps_helper::rmc.ew,gps_helper::rmc.ns));

        //---------------
        ui->te_gps_rmc->append(QString().sprintf("定位质量:%d",gps_helper::gga.fs));
        ui->te_gps_rmc->append(QString().sprintf("定位卫星数量:%d",gps_helper::gga.fix));
        ui->te_gps_rmc->append(QString().sprintf("水平精度因子:%f",gps_helper::gga.hdop));
        ui->te_gps_rmc->append(QString().sprintf("高度:%f",gps_helper::gga.altitude));

    }
    ui->te_gps_gsv->clear();
    ui->te_gps_gsv->append(QString().sprintf("test config:num :%d,cn:%d,time:%d",gps_helper::gps_param.num,gps_helper::gps_param.sni,gps_helper::gps_param.time));
    ui->te_gps_gsv->append(QString().sprintf("GPGSV Total:%d",gps_helper::GPGSV.Total));
    if(gps_helper::gps_parse_ok)
    {
        for(i=0;i<gps_helper::GPGSV.Total;i++)
        {
            ui->te_gps_gsv->append(QString().sprintf("ID:%04d:%04d",gps_helper::GPGSV.PRN[i],gps_helper::GPGSV.CN[i]));
            if( at_gps_cmd_block.is_factory_gps_snr_test==1)
            {
                if(gps_helper::GPGSV.PRN[i] == gps_helper::gps_param.num
                    && gps_helper::GPGSV.CN[i]>=gps_helper::gps_param.sni)
                {
                    //gps test passs
                    ui->pushButton_gps_stop->click();
                    at_gps_cmd_block.is_factory_gps_snr_test=0;
                    ui->progressBar_gps_test->setValue(0);
                    show_gps_result("PASS","00ff00");
                    save_gps_test_result("PASS", test_cmd_block.ota_imeir_text);
                }
            }
        }
        ui->te_gps_gsv->append(QString().sprintf("BDGSV Total:%d",gps_helper::BDGSV.Total));

        for(i=0;i<gps_helper::BDGSV.Total;i++)
        {
            ui->te_gps_gsv->append(QString().sprintf("ID:%04d:%04d",gps_helper::BDGSV.PRN[i],gps_helper::BDGSV.CN[i]));
        }
       // plot_update();
        bar_update();
    }
}

void mainWin::plot_init()
{
    ui->plot->xAxis->setRange(0, 25);//x轴范围
    ui->plot->yAxis->setRange(0, 60);//y轴范围
    ui->plot->xAxis->setLabel("x Axis");//设置x轴名称
    ui->plot->yAxis->setLabel("db");//设置y轴名称
    ui->plot->legend->setVisible(true);//设置每条曲线的说明可见,默认为不可见

    QFont legendFont = font();
    legendFont.setPointSize(10);//曲线说明的字体大小

    ui->plot->legend->setFont(legendFont);//使字体设置生效

    QVector<double> x1(10),y1(10);
    for(int i=0;i<10;i++)
    {
        x1[i]=0;
        y1[i]=x1[i];
    }

    ui->plot->addGraph();//画曲线

    ui->plot->graph(0)->setName(QString("gps"));//给曲线命名


    ui->plot->graph(0)->setData(x1, y1);

    ui->plot->graph(0)->setScatterStyle(QCPScatterStyle((QCPScatterStyle::ScatterShape)(2)));


    //标记三角形，点点等标记曲线
    //2为x标记，3为一个竖扛，4为空心圆，5为实心圆，6为空心正方形，7为空心棱形，8为＊，9为正空心三角形，10为倒空心三角形
    //11为里面带x的正方形，12为“田”，13，带x的圆
    QPen graphPen0;
    graphPen0.setColor(QColor(Qt::blue));
    graphPen0.setWidthF(1);
    ui->plot->graph(0)->setPen(graphPen0);//使用画笔绘制曲线
}
#define BAR_SIZE 20
void mainWin::bar_init()
{
    QVector<QString> labels(BAR_SIZE);
    QVector<double> values(BAR_SIZE);
    QVector<double> index(BAR_SIZE);
    ui->plot->xAxis->setRange(0, BAR_SIZE);//x轴范围
    ui->plot->yAxis->setRange(0, 60);//y轴范围
    for(int i=0;i<BAR_SIZE;++i)
    {
        labels[i]=QString::number(i+1,10);
        index[i]=i;
        values[i]=i+5;
    }
    g_bars=new QCPBars(this->ui->plot->xAxis,this->ui->plot->yAxis);
    g_bars->setData(index,values);

    //----------
    ui->plot->xAxis->setAutoTicks(false);
    ui->plot->xAxis->setAutoTickLabels(false);
    ui->plot->xAxis->setAutoTickStep(false);
    ui->plot->addPlottable(g_bars);
    //ui->plot->rescaleAxes();
    double wid=ui->plot->xAxis->range().upper-ui->plot->xAxis->range().lower;
    double cl=g_bars->width()+(1.0*wid-g_bars->width()*BAR_SIZE)/(BAR_SIZE-0);

    QVector<double> coor;
    for(int i=0;i<BAR_SIZE;++i)
        coor.append(ui->plot->xAxis->range().lower+i*cl+g_bars->width()/10);
    ui->plot->xAxis->setTickVector(coor);
    ui->plot->xAxis->setTickVectorLabels(labels);




    //----------
    ui->plot->replot();
}

void mainWin::plot_update()
{
    int i;
    QVector<double> x1(200),y1(200);
    if(gps_helper::gps_parse_ok)
    {
        for(i=0;i<gps_helper::GPGSV.Total;i++)
        {
            x1[i*4]=i*3;
            y1[i*4]=0;

            x1[i*4+1]=i*3;
            y1[i*4+1]=gps_helper::GPGSV.CN[i];

            x1[i*4+2]=i*3+1;
            y1[i*4+2]=gps_helper::GPGSV.CN[i];

            x1[i*4+3]=i*3+1;
            y1[i*4+3]=0;
        }
        for(i=0;i<gps_helper::BDGSV.Total;i++)
        {
            //x1[i]=i+gps_helper::GPGSV.Total;
            //y1[i+gps_helper::GPGSV.Total]=gps_helper::BDGSV.CN[i];
        }
    }
    ui->plot->graph(0)->setData(x1, y1);
    ui->plot->replot();
}

void mainWin::bar_update()
{
    int i;
    QVector<QString> labels(50);
    QVector<double> values(50);
    QVector<double> index(50);

    if(gps_helper::gps_parse_ok)
    {
        for(i=0;i<gps_helper::GPGSV.Total;i++)
        {
            index[i]=i+1;
            values[i]=gps_helper::GPGSV.CN[i];
            labels[i+1]=QString("G%1").arg(gps_helper::GPGSV.PRN[i]);
        }
        for(i=0;i<gps_helper::BDGSV.Total;i++)
        {
            index[i+gps_helper::GPGSV.Total]=i+1+gps_helper::GPGSV.Total;
            values[i+gps_helper::GPGSV.Total]=gps_helper::BDGSV.CN[i];
            labels[1+i+gps_helper::GPGSV.Total]=QString("B%1").arg(gps_helper::BDGSV.PRN[i]);
        }
        //----------------
        g_bars->setData(index, values);
        //----------
        ui->plot->xAxis->setTickVectorLabels(labels);
        //ui->plot->rescaleAxes();
        ui->plot->replot();
    }

}
void mainWin::com_read()
{
    //这个判断尤为重要,否则的话直接延时再接收数据,空闲时和会出现高内存占用
    static int cnt=0;
    static int tick;
    tick++;
    if (myCom->bytesAvailable()<=0){
        cnt=0;
        oldSize=0;
        return;
    }
    utilMan::sleep(1);//延时1毫秒
    if(myCom->bytesAvailable() > oldSize)
    {
        oldSize = myCom->bytesAvailable();
        cnt=0;
    }
    else
    {
        cnt++;
    }
    int delay;
    delay=15;
    if(cnt<delay)
    {
        return;
    }

    QByteArray combuf=myCom->readAll();
    QString com_temp_string=codec->toUnicode(combuf);

    com_at_process(combuf);
    show_com_rx(combuf);
    //显示收到的数据个数
    recvCount += combuf.size();
    ui->recvCount_Label->setText(QString("接收:%1 字节").arg(recvCount));
    if(!saveDataFlag)
    {
       QDir *temp = new QDir;
       QString runPath = QCoreApplication::applicationDirPath()+"/logs";
       //show_log(runPath);
       bool exist = temp->exists(runPath);
       if(exist)
       {
          //show_log("文件夹已经存在！");
       }
      else
      {
          bool ok = temp->mkdir(runPath);
          if( ok )
           {
              //show_log("文件夹创建成功");
           }
          else
           {
              show_log("文件夹创建失败");
           }
      }
       QString t= QDateTime::currentDateTime().toString("yyyy_MM_dd HH_mm_ss");
       QString fname=QString("%1/%2.log").arg(runPath).arg(t);
       //show_log(fname);
       file_save = new QFile(fname);
       if(file_save!=NULL&&file_save->open(QFile::WriteOnly|QFile::Append))
       {
           saveDataFlag=true;
       }
       else
       {
           show_log("log creat fail");
        }
    }
    if((saveDataFlag))
    {
        text_save = new QTextStream(file_save);
        *text_save << QString("【%1】 %2").arg(QTime::currentTime().toString("HH:mm:ss:zzz")).arg(com_temp_string) << "\r\n";
        delete text_save;
    }
    /*防止保存数据过大后挂掉*/
    if(saveDataFlag)
    {
        if(recvCount >= 5000000)
        {
            recvCount = 0;
            //ui->log_textEdit->clear();
            pmainlog->show_clear();
        }
    }
}

void mainWin::param_write()
{

}
void mainWin::com_write(QString sendStr)
{
    if(com_is_open==0) return ;
    if (!myCom->isOpen()) { return; }//串口没有打开

    QByteArray sendData=codec->fromUnicode(sendStr);
    int size=sendData.size();

    size=sendData.size();
    myCom->write(sendData);
    sendCount += size;
    ui->sendCount_Label->setText(QString("发送:%1 字节").arg(sendCount));
    if(config.is_com_print)
    {
       // show_log(QString("com write %1 bytes:").arg(sendStr.size()),QColor("black"),true);

        show_log(sendStr,QColor("green"),false);
    }

}

void mainWin::com_write(QByteArray sendData)
{
    if (!myCom->isOpen()) { return; }//串口没有打开
    myCom->write(sendData);
    sendCount += sendData.size();
    ui->sendCount_Label->setText(QString("发送:%1 字节").arg(sendCount));

    if(config.is_com_print)
    {
    }
    if(config.is_com_send_hex_print)
    {
        show_log(QString("com write %1 bytes:").arg(sendData.size()),QColor("black"),true);
        QString tempDataHex = utilMan::toHexStr(sendData);
        show_log(QString("%2").arg(tempDataHex),QColor("green"),false);
    }
}

/**
  *暂停显示槽函数
  */
void mainWin::on_isShow_CheckBox_stateChanged(int arg)
{
    config.is_com_send_hex_print = (arg==0?true:false);
}


/**
  *清空接收区槽函数
  */
void mainWin::on_emptyRecv_Button_clicked()
{
    //ui->log_textEdit->clear();
    pmainlog->show_clear();
    recvCount = 0;
    ui->recvCount_Label->setText(QString("接收:%1 字节").arg(recvCount));
    sendCount = 0;
    ui->sendCount_Label->setText(QString("发送:%1 字节").arg(sendCount));
}

/**
  *十六进制接收槽函数
  */
void mainWin::on_recv_16_CheckBox_clicked(bool checked)
{
    config.is_com_recv_hex_print = checked;
    setting_helper::app_config_save(config);
}


void mainWin::on_clearSendCount_Button_clicked()
{
    sendCount = 0;
    ui->sendCount_Label->setText(QString("发送:%1 字节").arg(sendCount));
}

void mainWin::on_print_com_clicked(bool checked)
{
    config.is_com_print=checked;
}
/*void readComThread::run()
{
    while(1)
    {
        utilMan::sleep(1000);
        qDebug("aaa");
    }
}*/



void mainWin::on_ButtonBinOpen_clicked()
{
    QFile       *file_send;
    // QTextStream *text_send;
    QDataStream *date_send;
    char bin_t[1024*500];
    QString sfile = QFileDialog::getOpenFileName(this,tr("升级固件"),".",tr("Images Files(*.bin);;All File(*)"));

    file_send = new QFile(sfile);
    QFileInfo file_info(sfile);
    if(file_send->open(QIODevice::ReadOnly))
    {
        //text_send = new QTextStream(file_send);
        date_send = new QDataStream(file_send);
        //QString sendingStr = text_send->readAll();
        // bin_writer.bindate = codec->fromUnicode(sendingStr);
        int size=date_send->readRawData(bin_t,500*1024);
        at_net_cmd_block.bindate=QByteArray(bin_t,size);
        at_net_cmd_block.binsize=at_net_cmd_block.bindate.size();
        delete date_send;
        show_log(QString("固件打开成功,共%1 bytes").arg(at_net_cmd_block.binsize));
        ui->lb_binfile->setText(QString("固件：%1\n大小:%2 bytes\n日期:%3").arg(sfile).arg(at_net_cmd_block.binsize).arg(file_info.lastModified().toString("yyyy-MM-dd HH:mm:ss")));
    }
    else
    {
        show_log("固件打开失败");
        ui->lb_binfile->setText("固件打开失败");
    }
}

void mainWin::on_ButtonBinWriteStart_clicked()
{
    QMessageBox::StandardButton  rb;
    if(config.is_boot_upgrade)
    {
        switch(QMessageBox::question(this,"注意 ",tr("此模式将烧写boot区固件，请确认"),
                                     QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes))
        {
        case QMessageBox::Yes:
            qDebug()<<"boot ok";
            break;
        default:
            qDebug()<<"boot fail";
            return;
            break;
        }
    }
    if(at_net_cmd_block.binsize==0)
    {
        this->on_ButtonBinOpen_clicked();
    }
    if(at_net_cmd_block.binsize==0)
    {
        ui->lb_fuse_resulte->setText("固件打开失败");
        rb=QMessageBox::information(this,"固件烧写","固件打开失败",QMessageBox::Ok,QMessageBox::Ok);
        if(rb == QMessageBox::Yes)
            return;
        return ;
    }
    if(config.is_boot_upgrade)
    {
        if(at_net_cmd_block.binsize>0x7000)
        {
            QMessageBox::information(this,"boot大小不对","boot大小异常",QMessageBox::Ok);
            return ;
        }
    }

    ui->pb_bin->setRange(0,at_net_cmd_block.binsize);
    ui->pb_bin->setValue(0);

    changeEnable(false);
    mode=work_mode_bin_update;
    at_net_cmd_block.timer->start();
    if(config.is_boot_upgrade)
    {
        at_net_cmd_block.state=at_gps_state_read_send;
        at_net_cmd_block.waitcnt=0;
        this->com_write(QString("#boot"));
        ui->lb_fuse_resulte->setText(QString("请求进入boot升级"));
    }
    else
    {
        this->com_write(QString("#reset"));
        utilMan::sleep(100);
        at_net_cmd_block.state=at_gps_state_read_wait;
    }
    at_net_cmd_block.packet_cnt=0;
    at_net_cmd_block.sendingPos=0;
    at_gps_cmd_block.gps_request_cnt=0;
    at_gps_cmd_block.gps_ack_cnt=0;

}

excel_rd mainWin::Excel_Read(QString ck_ceid,QString ck_imsi)
{
    excel_rd rd;
    QXlsx::Document xlsx("空中写号.xlsx");
    QXlsx::Format format1;
    format1.setFontColor(QColor(Qt::red));
    QXlsx::CellRange range = xlsx.dimension();
    int rowCounts = range.lastRow ();               //获取最后一行
    rd.read_status = -1;
    for(int i=2;i<=rowCounts;i++)
    {
        rd.ceid_excel = xlsx.read(i,excel_ceid_loc).toString();
        qDebug() << "excel line:"<<i<<"ceid:"<<rd.ceid_excel;
//        if(rd.imsi_excel == imsi_excel)
        if(rd.ceid_excel == ck_ceid)
        {
            rd.imsi_excel = xlsx.read(i,excel_imsi_loc).toString();
            if(rd.imsi_excel != ck_imsi)
            {
                QString ota_stt = xlsx.read(i,excel_ota_status_loc).toString();
//                QString check_imsi = xlsx.read(i,excel_check_imsi).toString();
                if(ota_stt == "ota success")
                {
                    rd.read_status = -4;
                    break;
                }
                rd.read_status = -5;
                break;
            }
            xlsx.write(i,excel_newnum_status_loc,"already used",format1);
            xlsx.saveAs("空中写号.xlsx");
            rd.ccid_excel = xlsx.read(i,excel_ccid_loc).toString();
            qDebug() << "空中写号.xlsx查找到数据";
            rd.cardid_excel = xlsx.read(i,excel_cardid_loc).toString();
            rd.newnum_status = xlsx.read(i,excel_newnum_status_loc).toString();
            if(rd.cardid_excel.length() < 13){
                rd.read_status = -6;
                xlsx.write(i,excel_check_cardnum,"no card",format1);
                break;
            }
            rd.read_status = 0;
            break;
        }
    }
//    if(rd.ccid_excel.length() == 0 && rd.cardid_excel.length() == 0)
//    {
//        if(rd.read_status = 1)
//        {
//            qDebug() << "空中写号.xlsx****没有****查找到数据";
//            rd.read_status = -1;
//        }
//    }
    if(rowCounts == -2)
    {
        qDebug() << "空中写号.xlsx 打开失败";
        rd.read_status = -2;
    }
    return rd;
}

int mainWin::Excel_check_ota(QString ck_ceid,QString ck_imsi,QString ck_ccid)
{
    int result = -1;
    excel_rd rd;
    QXlsx::Document xlsx("空中写号.xlsx");
    QXlsx::Format format1;
    QXlsx::Format format2;
    QXlsx::Format format;
    format1.setFontColor(QColor(Qt::red));
    format2.setFontColor(QColor(Qt::green));
    QXlsx::CellRange range = xlsx.dimension();
    int rowCounts = range.lastRow ();               //获取最后一行
    int i = 0;
    for(i = 2;i<=rowCounts;i++)
    {
        rd.ceid_excel = xlsx.read(i,excel_ceid_loc).toString();
        qDebug() << "excel line:"<<i<<"ceid:"<<rd.ceid_excel;
        if(rd.ceid_excel == ck_ceid)
        {
            rd.ccid_excel = xlsx.read(i,excel_ccid_loc).toString();
            qDebug() << "p310开卡.xlsx查找到数据";
            rd.imsi_excel = xlsx.read(i,excel_imsi_loc).toString();
//            rd.status_excel = xlsx.read(i,excel_newnum_status_loc).toString();
            rd.cardid_excel = xlsx.read(i,excel_cardid_loc).toString();
            rd.newnum_status = xlsx.read(i,excel_newnum_status_loc).toString();
//            rd.new_ccid = xlsx.read(i,excel_newccid_loc).toString();
            if(rd.imsi_excel != ck_imsi)
            {
                xlsx.write(i,excel_ota_status_loc,"ota success",format1);
                if(1 == test_cmd_block.ota_imei_write){
                   xlsx.write(i,excel_imei_loc,test_cmd_block.ota_imeir_text,format1);
                }
                if(ck_ccid != rd.ccid_excel){
                    result = 2;
                    format = format2;
                }else{
                    result = 1;
                    format = format1;
                }
                xlsx.write(i,excel_newccid_loc,ck_ccid,format);
                xlsx.write(i,excel_check_imsi,ck_imsi,format);
                xlsx.saveAs("空中写号.xlsx");
            }
            break;
        }
        if(rd.ceid_excel.length() == 0 && rd.imsi_excel.length() == 0)
        {
            qDebug() << "空中写号.xlsx****没有****查找到数据";
            result = -1;
            break;
        }
    }
    return result;
}

void mainWin::on_at_timer()
{
    if(test_cmd_block.test_enable)
    {
        switch (test_cmd_block.state) {
        case at_state_ccid_read_send:
            test_cmd_block.send_cnt++;
            if(test_cmd_block.send_cnt<=4)
            {
                ui->lb_eid_read_data->setText(QString("ceid"));
                ui->lb_ccid_read_data->setText(QString("ccid"));
                ui->lb_imsi_read_data->setText(QString("imsi"));
                this->com_write(QString("AT+CCID\r\n"));
                ui->lb_check_ota_result->setText(QString("读取中%1...").arg(test_cmd_block.send_cnt));
                test_cmd_block.state=at_state_ccid_read_wait;
                test_cmd_block.wait_time_cnt=0;
            }
            else
            {
                test_cmd_block.state=at_state_ccid_read_fail;
                ui->lb_check_ota_result->setText(QString("读取失败"));
                ui->lb_ccid_read_data->setText(QString("ERROR"));
            }

            break;
        case at_state_ccid_read_wait:
            test_cmd_block.wait_time_cnt++;
            if(test_cmd_block.wait_time_cnt>AT_WAIT_TIME_DELAY_2s)
            {
                test_cmd_block.state=at_state_ccid_read_send;
                test_cmd_block.wait_time_cnt = 0;
            }
            break;
        case at_state_ccid_read_ack:
            ui->lb_check_ota_result->setText(QString("ccid读取成功"));
            ui->lb_ccid_read_data->setText(test_cmd_block.ccid_text);
            test_cmd_block.state=at_state_imsi_read_send;
            break;
        case at_state_ccid_read_fail:
            //TODO can be null
            break;
        case at_state_imsi_read_send:
            this->com_write(QString("AT+CIMI\r\n"));
            ui->lb_check_ota_result->setText(QString("读取中%1...").arg(test_cmd_block.send_cnt));
            test_cmd_block.state=at_state_imsi_read_wait;
            break;
        case at_state_imsi_read_wait:
            test_cmd_block.wait_time_cnt++;
            if(test_cmd_block.wait_time_cnt>AT_WAIT_TIME_DELAY_4s)
            {
                ui->lb_check_ota_result->setText(QString("读取imsi失败"));
                ui->lb_imsi_read_data->setText(QString("ERROR"));
                test_cmd_block.state=at_state_ccid_read_send;
                test_cmd_block.wait_time_cnt = 0;
            }
            break;
        case at_state_imsi_read_ack:
            ui->lb_check_ota_result->setText(QString("imsi读取成功"));
            ui->lb_imsi_read_data->setText(test_cmd_block.imsi_text);
            test_cmd_block.state=at_state_ceid_read_send;
            break;
        case at_state_imsi_read_fail:
            //TODO can be null
            break;
        case at_state_ceid_read_send:
            this->com_write(QString("AT+CEID\r\n"));
            ui->lb_check_ota_result->setText(QString("读取CEID..."));
            test_cmd_block.state=at_state_ceid_read_wait;
            break;
        case at_state_ceid_read_wait:
            test_cmd_block.wait_time_cnt++;
            if(test_cmd_block.wait_time_cnt>AT_WAIT_TIME_DELAY_4s)
            {
                ui->lb_check_ota_result->setText(QString("读取CEID失败"));
                ui->lb_eid_read_data->setText(QString("ERROR"));
                test_cmd_block.state=at_state_ccid_read_send;
                test_cmd_block.wait_time_cnt = 0;
            }
            break;
        case at_state_ceid_read_ack:

            id_cnt_update();
            ui->lb_check_ota_result->setText(QString("读取成功"));
            ui->lb_eid_read_data->setText(test_cmd_block.ceid_text);
            if(test_cmd_block.ota_enable == 1)
            {
                excel_rd excel = Excel_Read(test_cmd_block.ceid_text,test_cmd_block.imsi_text);
                test_cmd_block.state = at_state_post_result;
                if(excel.read_status == 0){
                    client.post_p310(excel.ceid_excel,excel.imsi_excel,excel.cardid_excel);
                    ui->lb_check_ota_result->setText(QString("post info"));
                }else if(excel.read_status == -1){
                    ui->lb_check_ota_result->setText(QString("未找到该设备信息"));
                    ui->lb_check_ota_result->setStyleSheet("background-color:red");
                }else if(excel.read_status == -2){
                    ui->lb_check_ota_result->setText(QString("空中写号.xlsxb不存在"));
                    ui->lb_check_ota_result->setStyleSheet("background-color:red");
                }else if(excel.read_status == -4){
                    ui->lb_check_ota_result->setText(QString("已写号，且检验成功"));
                    ui->lb_check_ota_result->setStyleSheet("background-color:red");
                }else if(excel.read_status == -5){
                    ui->lb_check_ota_result->setText(QString("已写号，未检验，请直接点击检测"));
                    ui->lb_check_ota_result->setStyleSheet("background-color:red");
                }else if(excel.read_status == -6){
                    ui->lb_check_ota_result->setText(QString("没有分配卡号"));
                    ui->lb_check_ota_result->setStyleSheet("background-color:red");
                }
                test_cmd_block.ota_enable = 0;
//                client.post_p310(test_cmd_block.ceid_text,test_cmd_block.imsi_text,excel.cardid_excel);
//                post_p310_win(test_cmd_block.ceid_text,test_cmd_block.imsi_text);
            }
            else if(test_cmd_block.check_enable == 1)
            {
                test_cmd_block.check_enable = 0;
                int result =Excel_check_ota(test_cmd_block.ceid_text,test_cmd_block.imsi_text,test_cmd_block.ccid_text);
                if(2 == result)
                {
                    ui->lb_check_ota_result->setText(QString("成功"));
                    ui->lb_check_ota_result->setStyleSheet("background-color:green");
                    ui->te_imei_write_text->clear();
                }
                else if(1 == result)
                {
                    ui->lb_check_ota_result->setText(QString("成功,CCID未改变"));
                    ui->lb_check_ota_result->setStyleSheet("background-color:green");
                    ui->te_imei_write_text->clear();
                }
                else
                {
                    ui->lb_check_ota_result->setText(QString("失败"));
                    ui->lb_check_ota_result->setStyleSheet("background-color:red");

                }
                test_cmd_block.state=at_state_idle;
            }
            else
            {
                g_db.ccid_update_or_insert(test_cmd_block.ccid_text,test_cmd_block.ceid_text,test_cmd_block.imsi_text);
                test_cmd_block.state=at_state_idle;
            }
            break;
        case at_state_post_result:
            if(post_flag == 1)
            {
                post_flag = 0;
                ui->lb_ota_post_data->setText(ota_post_data);
                ui->lb_ota_post_result->setText(ota_post_result);
                if(ota_post_result.contains("200"))
                {
                    ui->lb_ota_post_result->setStyleSheet("background-color:green");
                    test_cmd_block.state = at_state_ota_sleep;
                }
                else
                {
                    ui->lb_ota_post_result->setStyleSheet("background-color:red");
                    test_cmd_block.state=at_state_idle;
                }
            }
            break;
        case at_state_ota_sleep:
            {
                test_cmd_block.wait_time_cnt++;
                if(test_cmd_block.wait_time_cnt > OTA_SLEEP SECONDS){
                    test_cmd_block.state=at_state_ccid_read_send;
                    test_cmd_block.send_cnt=0;
                    test_cmd_block.test_enable=1;
                    test_cmd_block.check_enable = 1;
                    test_cmd_block.wait_time_cnt = 0;
                }else{
                    ui->lb_check_ota_result->setText(QString().sprintf("等待 %d 秒后检测", (OTA_SLEEP - test_cmd_block.wait_time_cnt/100)));
                    ui->lb_check_ota_result->setStyleSheet("background-color:yellow");
                }
            }
            break;
        case at_state_ceid_read_fail:
            break;
        case at_state_gsensor_test_wait:
            test_cmd_block.wait_time_cnt++;
            if(test_cmd_block.wait_time_cnt>AT_WAIT_TIME_DELAY_4s)
            {
                test_cmd_block.state=at_state_idle;
                show_gsensor_result("TIMEOUT","ff0000");
            }
            break;
        default:
            break;
        }
    }
    if(at_net_cmd_block.net_loop_enable)
    {
        switch (at_net_cmd_block.state) {
        case at_net_state_cpin_send:
            this->com_write(QString("AT+CPIN?\r\n"));
            at_net_cmd_block.state=at_net_state_cpin_wait;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_DEFAULT;
            break;
        case at_net_state_cpin_wait:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                at_net_cmd_block.state=at_net_state_cpin_send;
            }
            break;
        case at_net_state_cpin_ack:
            at_net_cmd_block.state=at_net_state_cgatt_send;
            break;
        case at_net_state_cgatt_send:
            this->com_write(QString("AT+CGATT?\r\n"));
            at_net_cmd_block.state=at_net_state_cgatt_wait;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_DEFAULT;
            break;
        case at_net_state_cgatt_wait:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                at_net_cmd_block.state=at_net_state_cgatt_send;
            }

            break;
        case at_net_state_cgatt_ack:
            id_cnt_update();
            at_net_cmd_block.state=at_net_state_qihead_send;
            break;
        case at_net_state_qihead_send:
            this->com_write(QString("AT+QIHEAD=1\r\n"));
            at_net_cmd_block.state=at_net_state_qihead_wait;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_DEFAULT;
            break;
        case at_net_state_qihead_wait:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                at_net_cmd_block.state=at_net_state_qihead_send;
            }
            break;
        case at_net_state_qihead_ack:
            at_net_cmd_block.state=at_net_state_qiopen_send;
            break;
        case at_net_state_qiopen_send:
        {
            QString s=ui->le_qiopen->text();
            QString ss=QString("AT+QIOPEN=");
            ss.append(s);
            ss.append("\r\n");
            this->com_write(ss);
            //-----------
            at_net_cmd_block.state=at_net_state_qiopen_wait;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_CONNECT;
        }
            break;
        case at_net_state_qiopen_wait:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                at_net_cmd_block.state=at_net_state_error;
            }

            break;
        case at_net_state_qiopen_ack:
            at_net_cmd_block.state=at_net_state_qisend_send;
            break;
        case at_net_state_qisend_send:
            this->com_write(QString("AT+QISEND\r\n"));
            at_net_cmd_block.state=at_net_state_prompt_wait;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_DEFAULT;
            at_net_cmd_block.net_send_cnt++;
            break;
        case at_net_state_prompt_wait:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                at_net_cmd_block.state=at_net_state_error;
            }
            break;
        case at_net_state_prompt_ack:
            at_net_cmd_block.state=at_net_state_qidata_send;
            break;
        case at_net_state_qidata_send:
        {
            QString s=ui->le_net_send_content->text();
            s.append("\x1a");
            this->com_write(s);
            at_net_cmd_block.state=at_net_state_qidata_wait;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_SEND;
        }
            break;
        case at_net_state_qidata_wait:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                at_net_cmd_block.state=at_net_state_error;
            }
            break;
        case at_net_state_qidata_ack:
            at_net_cmd_block.state=at_net_state_qidata_delay;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_DELAY;
            at_net_cmd_block.net_send_ok_cnt++;

            break;
        case at_net_state_qidata_delay:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                if(at_gps_cmd_block.gps_loop_enable)
                {
                   at_gps_cmd_block.state=at_gps_state_open_send;
                   at_net_cmd_block.state=at_net_state_wait_gps;
                }
                else
                {
                   at_net_cmd_block.state=at_net_state_qisend_send;
                }
            }
            break;

        case at_net_state_qideact_send:
            this->com_write(QString("AT+QIDEACT\r\n"));
            at_net_cmd_block.state=at_net_state_qideact_wait;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_QIDEACT;
            break;
        case at_net_state_qideact_wait:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                at_net_cmd_block.state=at_net_state_qideact_ack;
            }
            break;
        case at_net_state_qideact_ack:
            at_net_cmd_block.state=at_net_state_qiact_send;
            break;
        case at_net_state_qiact_send:
            this->com_write(QString("AT+QIACT\r\n"));
            at_net_cmd_block.state=at_net_state_qiact_wait;
            at_net_cmd_block.waitcnt=AT_WAIT_TIME_QIACT;
            break;
        case at_net_state_qiact_wait:
            if(at_net_cmd_block.waitcnt)
            {
                at_net_cmd_block.waitcnt--;
            }
            else
            {
                at_net_cmd_block.state=at_net_state_qiact_ack;
            }

            break;
        case at_net_state_qiact_ack:
            at_net_cmd_block.state=at_net_state_cpin_send;
            break;
        case at_net_state_wait_gps:
            if(at_gps_cmd_block.gps_loop_enable)
            {
                if(at_gps_cmd_block.state==at_gps_state_end)
                {
                    at_net_cmd_block.state=at_net_state_qisend_send;
                }
            }
            else
            {
                at_net_cmd_block.state=at_net_state_qisend_send;
            }
            break;
        case at_net_state_error:
            show_log("NET ERROR!!!!!");
            at_net_cmd_block.state=at_net_state_error_ack;
            at_net_cmd_block.net_error_cnt++;
            break;
        case at_net_state_error_ack:
            at_net_cmd_block.state=at_net_state_qideact_send;
            break;
        default:
            break;
        }
    }

    if(at_gps_cmd_block.gps_loop_enable)
    {
        switch (at_gps_cmd_block.state) {
        case  at_net_state_cpin_send:
            if(config.is_gps_check_sim && at_gps_cmd_block.cpin_is_tested==0)
            {
                this->com_write(QString("AT+CPIN?\r\n"));
                at_gps_cmd_block.state=at_net_state_cpin_wait;
                at_gps_cmd_block.waitcnt=AT_WAIT_TIME_DEFAULT;
            }
            else
            {
                at_gps_cmd_block.state=at_gps_state_open_send;
            }

            break;
        case  at_net_state_cpin_wait:
            if(at_gps_cmd_block.waitcnt)
            {
                at_gps_cmd_block.waitcnt--;
            }
            else
            {
                at_gps_cmd_block.state=at_net_state_cpin_send;
                at_gps_cmd_block.cpin_test_cnt++;
                if(at_gps_cmd_block.cpin_test_cnt>=5)
                {
                    ui->pushButton_gps_stop->click();
                    show_gps_result("FAIL:SIM","ff0000");
                    at_gps_cmd_block.is_factory_gps_snr_test=0;
                    ui->progressBar_gps_test->setValue(0);
                    save_gps_test_result("FAIL:SIM", test_cmd_block.ota_imeir_text);
                }
            }
            break;
        case  at_net_state_cpin_ack:
            at_gps_cmd_block.state=at_gps_state_open_send;
            at_gps_cmd_block.cpin_is_tested=1;
            break;
        case  at_gps_state_open_send:
            if(at_gps_cmd_block.gps_is_opened==0 && config.is_gps_autoopen )
            {
                this->com_write(QString("AT+QGNSSC=1\r\n"));
                at_gps_cmd_block.state=at_gps_state_open_wait;
                at_gps_cmd_block.waitcnt=GPS_OPEN_WAIT_TIME;
                at_gps_cmd_block.gps_is_opened=1;
            }
            else
            {
                at_gps_cmd_block.state=at_gps_state_read_send;
            }

            break;
        case  at_gps_state_open_wait:
            qDebug()<<QString("gps open wait");
            if(at_gps_cmd_block.waitcnt>0)
            {
                at_gps_cmd_block.waitcnt--;
            }
            else
            {
                at_gps_cmd_block.state=at_gps_state_open_delay;
                at_gps_cmd_block.waitcnt=GPS_OPEN_DELAY_TIME;
            }
            break;
        case at_gps_state_open_ack:
            at_gps_cmd_block.state=at_gps_state_open_delay;
            at_gps_cmd_block.waitcnt=GPS_OPEN_DELAY_TIME;
            break;
        case  at_gps_state_open_delay:
            if(at_gps_cmd_block.waitcnt>0)
            {
                at_gps_cmd_block.waitcnt--;
            }
            else
            {
                at_gps_cmd_block.state=at_gps_state_read_send;
            }
            break;
        case at_gps_state_read_send:
            qDebug()<<QString("gps read send");
            this->com_write(QString("AT+QGNSSRD?\r\n"));
            at_gps_cmd_block.state=at_gps_state_read_wait;
            at_gps_cmd_block.waitcnt=GPS_READ_WAIT_TIME;
            at_gps_cmd_block.gps_request_cnt++;
            break;
        case at_gps_state_read_wait:
            if(at_gps_cmd_block.waitcnt>0)
            {
                at_gps_cmd_block.waitcnt--;
            }
            else
            {
                at_gps_cmd_block.state=at_gps_state_read_delay;
                at_gps_cmd_block.waitcnt=GPS_READ_DELAY_TIME;
            }
            break;
        case at_gps_state_read_ack:
           qDebug()<<QString("gps read ack");
            at_gps_cmd_block.state=at_gps_state_read_delay;
            at_gps_cmd_block.waitcnt=GPS_READ_DELAY_TIME;
            at_gps_cmd_block.gps_ack_cnt++;
            break;
        case at_gps_state_read_delay:
            if(at_gps_cmd_block.waitcnt>0)
            {
                at_gps_cmd_block.waitcnt--;
            }
            else
            {
                 qDebug()<<QString("gps read delay");
                at_gps_cmd_block.state=at_gps_state_end;
            }
            break;
        case at_gps_state_end:
            qDebug()<<QString("at_gps_state_end");
            if(at_net_cmd_block.net_loop_enable==0)
            {
                at_gps_cmd_block.state=at_net_state_cpin_send;
            }
            break;
        case at_gps_state_error_poweroff:
        case at_gps_state_error_other:
            at_gps_cmd_block.state=at_gps_state_open_send;
            at_gps_cmd_block.gps_is_opened=0;
            break;

        case at_gps_state_agps_send:
            qDebug()<<QString("agps send");
            this->com_write(QString("AT+QAGPS=1\r\n"));
            at_gps_cmd_block.state=at_gps_state_agps_wait;
            break;
        case at_gps_state_agps_wait:
            break;
        default:
            break;
        }
    }
//    else
//    {
//        at_gps_cmd_block.state=at_gps_state_end;
//    }

}

void mainWin::on_ButtonBinWriteStop_clicked()
{
    changeEnable(true);
    //at_cmd_block.state=at_gps_state_idle;
    at_net_cmd_block.timer->stop();
    ui->lb_fuse_resulte->setText("烧写中止");
    ui->pb_bin->setValue(0);
}

void mainWin::on_recv_16_CheckBox_clicked()
{

}


void mainWin::processFinished(QString result)
{
    show_log(QString("com %1").arg(result));
}

void mainWin::processProgress(int progress)
{
    show_log(QString("com percent:%1").arg(progress));
}

void mainWin::process_key_fuse_start()
{
    this->activateWindow();
    if(!is_bin_writing)
    {
        on_ButtonBinWriteStart_clicked();
    }
    else
    {
        show_log("已经烧写中....");
    }
}

void mainWin::process_key_fuse_stop()
{
    show_log("烧写停止");
    this->activateWindow();
    on_ButtonBinWriteStop_clicked();
}

void mainWin::process_key_test()
{
    QPushButton *tb  = (QPushButton*) sender();
    int i;
    mode=work_mode_test;
    for(i=0;i<cmd_test_config.num_all;i++)
    {
        if(tb==cmd_test_config.cmds[i].button)
        {   QString cmd= cmd_test_config.cmds[i].textedit->toPlainText();
            utilMan::STR_AA(cmd);
            show_log(QString("执行:%1").arg(cmd_test_config.cmds[i].caption));
            this->com_write(cmd);
            param_state=param_state_read;
            break;
        }
    }
}

void mainWin::param_edit_enable(bool enable)
{
    //ui->le_qr_code->setEnabled(!enable);

}

void mainWin::process_show_slot()
{
    this->activateWindow();
    this->showNormal();
    //this->show();
    //this->raise();
    //this->move(pmainlog->geometry().topRight().x()+16,pmainlog->y());
}

void mainWin::on_send_16_CheckBox_clicked(bool checked)
{
    config.is_com_send_hex_print =checked;
    setting_helper::app_config_save(config);
}

void mainWin::on_cbprintcom_clicked(bool checked)
{
    config.is_com_print =checked;
    setting_helper::app_config_save(config);
}
void mainWin::on_cb_com_cr_show_clicked(bool checked)
{
    config.is_com_cr_show =checked;
    setting_helper::app_config_save(config);
}
void mainWin::on_cb_config_ccid_read_clicked(bool checked)
{
    config.is_imei_write_then_ccid_read=checked;
    setting_helper::app_config_save(config);
}

void mainWin::on_cb_config_imei_repeated_check_clicked(bool checked)
{
    config.is_imei_repeated_check=checked;
    setting_helper::app_config_save(config);
}

void mainWin::on_cb_config_gps_autoopen_clicked(bool checked)
{
    config.is_gps_autoopen=checked;
    setting_helper::app_config_save(config);
}
void mainWin::on_cb_config_imei_is_infront_clicked(bool checked)
{
    config.is_imei_infront=checked;
    setting_helper::app_config_save(config);
}

void mainWin::on_cb_config_gps_checksim_clicked(bool checked)
{
    config.is_gps_check_sim=checked;
    setting_helper::app_config_save(config);
}
void mainWin::on_tabMain_currentChanged(int index)
{
    //show_log(QString("tab changed%1").arg(index));
    switch (index) {
    case 0:
        mode=work_mode_param;
        break;
    case 1:
        mode=work_mode_bin_update;
        break;
    case 2:
        mode=work_mode_test;
        break;
    case 3:
        mode=work_mode_uart;
        break;
        break;
    case 4:
        mode=work_mode_line_test;
        break;
    default:
        break;
    }
}


void mainWin::on_CheckBoxIsBoot_clicked(bool checked)
{
    config.is_boot_upgrade=checked;
    setting_helper::app_config_save(config);
}



void mainWin::on_le_qr_code_editingFinished()
{
    //    //show_log("input finish",QColor("red"),true);
    //    show_param_w_clear();
    //    show_param_config();
    //   // QString qrcode=ui->le_qr_code->text();
    //    param_t param_temp;
    //    if(access_helper::param_get_use_qrcode(qrcode,param_temp))
    //    {
    //        ui->le_device_id->setText(param_temp.deviceId);
    //        ui->le_device_secret->setText(param_temp.deviceSecret);
    //        ui->le_device_name->setText(param_temp.deviceName);
    //        //ui->le_qr_code->setText(param_temp.qr_code);
    //        show_result("ready","0000ff");
    //    }
    //    else
    //    {
    //        show_result("未找到对应qr_code","ff0000");
    //    }
}

void mainWin::on_toolBox_currentChanged(int index)
{
    /*
    QString s;
    s.sprintf("index:%d",index);
    show_log(s);
    */
    switch (index) {
    case 1:
        param_edit_enable(true);
        on_Button_Config_Read_clicked();
        break;
    default:
        param_edit_enable(false);
        break;
    }
}

void mainWin::on_Button_Config_Read_clicked()
{
    param_config=setting_helper::param_read();
    QMessageBox::about(NULL, "读取", "烧号配置读取完成");
}

void mainWin::on_Button_Config_Save_clicked()
{
    QMessageBox::about(NULL, "保存", "烧号配置保存成功");
}


void mainWin::on_pb_test_lock_check_clicked()
{
    this->com_write(QString("AT+CPOF\r\n"));
}

void mainWin::on_pb_test_reboot_clicked()
{
    this->com_write(QString("ATI\r\n"));
}

void mainWin::on_pb_test_lockopen_clicked()
{
    this->com_write(QString("AT+CFUN=0\r\n"));
}

void mainWin::on_pb_test_inquery_clicked()
{
    this->com_write(QString("AT+GSN\r\n"));
}

void mainWin::on_pb_test_debug_on_clicked()
{
    this->com_write(QString("ATE1\r\n"));
}

void mainWin::on_pb_test_debug_off_clicked()
{
    this->com_write(QString("ATE0\r\n"));
}

void mainWin::on_pb_test_param_read_clicked()
{
    this->com_write(QString("AT+CCLK?\r\n"));
}

void mainWin::on_pb_test_lock_reset_clicked()
{
    this->com_write(QString("AT+CFUN=1\r\n"));
}

void mainWin::on_pb_test_debug_on_AT_clicked()
{
    this->com_write(QString("AT+CGATT?\r\n"));
}

void mainWin::on_pb_test_debug_off_AT_clicked()
{
    this->com_write(QString("AT+CGATT=1\r\n"));
}

void mainWin::on_pb_test_debug_on_GPS_clicked()
{
    this->com_write(QString("#debugon gps"));
}

void mainWin::on_pb_test_debug_off_GPS_clicked()
{
    this->com_write(QString("#debugoff gps"));
}


void mainWin::on_pb_test_debug_off_AXIS_clicked()
{
    this->com_write(QString("AT+QILOCIP\r\n"));
}

void mainWin::on_pb_test_motor_f_clicked()
{
    QString s=ui->le_qiopen->text();
    QString ss=QString("AT+QIOPEN=");
    ss.append(s);
    ss.append("\r\n");
    this->com_write(ss);

}

void mainWin::on_pb_test_motor_b_clicked()
{
    this->com_write(QString("AT+QISEND\r\n"));
}

void mainWin::on_pb_test_heartbeat_send_clicked()
{
    this->com_write(QString("AT+QICLOSE\r\n"));
}

void mainWin::on_pb_test_power_info_clicked()
{
    this->com_write(QString("AT+CBC?\r\n"));
}

void mainWin::on_pb_test_log_r_clicked()
{
    this->com_write(QString("AT+CGPADDR=1,2,3,4,5,6,7\r\n"));
}

void mainWin::on_pb_test_param_clear_clicked()
{
    this->com_write(QString("#paramclear"));
}

void mainWin::on_pb_test_gps_info_clicked()
{
    this->com_write(QString("#postion"));
}

void mainWin::on_pb_test_savemode_clicked()
{
    this->com_write(QString("ATV1\r\n"));
}

void mainWin::on_pb_test_runmode_clicked()
{
    this->com_write(QString("AT+CCLK?\r\n"));
}


void mainWin::on_pb_test_pcb_check_clicked()
{
    this->com_write(QString("AT\r\n"));
}

void mainWin::on_pb_test_savemode_2_clicked()
{
    this->com_write(QString("ATV0\r\n"));
}

void mainWin::on_pb_test_savemode_3_clicked()
{
    this->com_write(QString("AT+CMEE=0\r\n"));
}

void mainWin::on_pb_test_savemode_5_clicked()
{
    this->com_write(QString("AT+CMEE=1\r\n"));
}

void mainWin::on_pb_test_savemode_4_clicked()
{
    this->com_write(QString("AT+CMEE=2\r\n"));
}

void mainWin::on_pushButton_clicked()
{
    this->com_write(QString("AT+CGACT?\r\n"));
}

void mainWin::on_pushButton_2_clicked()
{
    this->com_write(QString("AT+CGACT=1\r\n"));
}
void mainWin::on_pb_test_debug_off_AT_2_clicked()
{
    this->com_write(QString("AT+CGACT=0\r\n"));
}
void mainWin::on_pushButton_3_clicked()
{
    this->com_write(QString("AT+CGACT=0\r\n"));
}


void mainWin::on_pb_test_debug_on_AXIS_clicked()
{
    this->com_write(QString("AT+QIACT\r\n"));
}

void mainWin::on_pb_test_debug_on_AXIS_2_clicked()
{
    this->com_write(QString("AT+QIDEACT\r\n"));
}

void mainWin::on_pb_test_motor_b_2_clicked()
{
    QString s=ui->le_net_send_content->text();
    s.append("\x1a");
    this->com_write(s);

}

void mainWin::on_pb_test_heartbeat_send_2_clicked()
{
    this->com_write(QString("AT+QISTAT\r\n"));
}

void mainWin::on_pb_test_debug_on_AXIS_3_clicked()
{
    this->com_write(QString("AT+QIHEAD=1\r\n"));
}

void mainWin::on_pb_test_debug_on_AXIS_4_clicked()
{
    this->com_write(QString("AT+QIHEAD=0\r\n"));
}

void mainWin::on_pb_gps_1_clicked()
{
    this->com_write(QString("AT+QGNSSC=1\r\n"));
}

void mainWin::on_pb_gps_2_clicked()
{
    this->com_write(QString("AT+QGNSSC=0\r\n"));
}

void mainWin::on_pb_gps_3_clicked()
{
    this->com_write(QString("AT+QGNSSRD?\r\n"));
    at_net_cmd_block.state=at_gps_state_read_wait;
    at_net_cmd_block.waitcnt=GPS_READ_WAIT_TIME;
}

void mainWin::on_pb_gps_4_clicked()
{
    this->com_write(QString("AT+QGNSSRD=\"NMEA/RMC\"\r\n"));
    at_net_cmd_block.state=at_gps_state_read_wait;
    at_net_cmd_block.waitcnt=GPS_READ_WAIT_TIME;
}

void mainWin::on_pb_gps_5_clicked()
{
    this->com_write(QString("AT+QAGPS=1\r\n"));
}

void mainWin::on_pb_gps_6_clicked()
{
    this->com_write(QString("AT+QCELLLOC=1\r\n"));
}

void mainWin::on_pushButton_4_clicked()
{
    this->com_write(QString("AT+CPIN?\r\n"));
}

void mainWin::on_pushButton_6_clicked()
{
    this->com_write(QString("AT+CREG?\r\n"));
}

void mainWin::on_pushButton_7_clicked()
{
    this->com_write(QString("AT+CGREG?\r\n"));
}

void mainWin::on_pushButton_8_clicked()
{
    this->com_write(QString("AT+CSQ\r\n"));
}

void mainWin::on_pushButton_9_clicked()
{
    this->com_write(QString("AT+CCID\r\n"));
}

void mainWin::on_pushButton_10_clicked()
{
    this->com_write(QString("AT+CIMI\r\n"));
}

void mainWin::on_pushButton_11_clicked()
{
    this->com_write(QString("AT+EGMR=2,7\r\n"));
}

void mainWin::on_pb_test_qsclk_clicked()
{
    this->com_write(QString("AT+QSCLK=0\r\n"));
}

void mainWin::on_pushButton_12_clicked()
{
    QString s=ui->le_ping_host->text();
    QString ss=QString("AT+QPING=\"");
    ss.append(s);
    ss.append("\"\r\n");
    this->com_write(ss);
}


void mainWin::on_cb_gps_loop_clicked(bool checked)
{
    at_gps_cmd_block.gps_loop_enable=checked;
    at_gps_cmd_block.state=at_gps_state_end;
    qDebug()<<QString().sprintf("gps loop:%d",(int)checked);
}


void mainWin::on_cb_net_loop_clicked(bool checked)
{
    at_net_cmd_block.net_loop_enable=checked;
    if(at_net_cmd_block.net_loop_enable)
    {
        at_net_cmd_block.state=at_net_state_cpin_send;
    }
    qDebug()<<QString().sprintf("net loop:%d",(int)checked);
}

void mainWin::on_cb_net_loop_clicked()
{

}

void mainWin::on_pushButton_14_clicked()
{
    this->com_write(QString("AT+CGDCONT?\r\n"));
}

void mainWin::on_pushButton_13_clicked()
{
    QString s=ui->le_cgdcont->text();
    QString ss=QString("AT+CGDCONT=");
    ss.append(s);
    ss.append("\r\n");
    this->com_write(ss);
}

void mainWin::on_pushButton_gps_start_clicked()
{
    at_gps_cmd_block.gps_loop_enable=true;
//    at_gps_cmd_block.state=at_net_state_cpin_send;
    at_gps_cmd_block.gps_is_opened=0;
    at_gps_cmd_block.is_factory_gps_snr_test=1;
    at_gps_cmd_block.gps_test_life=0;
    at_gps_cmd_block.cpin_test_cnt=0;
    at_gps_cmd_block.cpin_is_tested=0;
    test_cmd_block.state = at_state_imei_read_wait;
    test_cmd_block.gps_test_imei_get = 1;
    QString buf=QString().sprintf("AT+EGMR=2,7\r\n");
    this->com_write(buf);
    show_gps_result("testing...","000000");
    ui->pushButton_gps_start->setEnabled(false);
    ui->pushButton_gps_stop->setEnabled(true);
    memset(&gps_helper::GPGSV, 0x00,sizeof(gsv_t));
}

void mainWin::on_pushButton_gps_stop_clicked()
{
   at_gps_cmd_block.is_factory_gps_snr_test=0;
   ui->progressBar_gps_test->setValue(0);
   at_gps_cmd_block.gps_loop_enable=false;
   show_gps_result("(* *)","000000");
   ui->pushButton_gps_start->setEnabled(true);
   ui->pushButton_gps_stop->setEnabled(false);
}
void mainWin::on_pushButton_agps_start_clicked()
{
    at_gps_cmd_block.gps_loop_enable=true;
    at_gps_cmd_block.state=at_gps_state_agps_send;
    at_gps_cmd_block.is_factory_agps_test=1;
    at_gps_cmd_block.agps_test_life=0;
    show_agps_result("Testing...","000000");
    ui->pushButton_agps_start->setEnabled(false);
    ui->pushButton_agps_stop->setEnabled(true);
}

void mainWin::on_pushButton_agps_stop_clicked()
{
    at_gps_cmd_block.is_factory_agps_test=0;
    ui->progressBar_agps_test->setValue(0);
    at_gps_cmd_block.gps_loop_enable=false;
    show_agps_result("(* *)","000000");
    ui->pushButton_agps_start->setEnabled(true);
    ui->pushButton_agps_stop->setEnabled(false);
}
void mainWin::on_pushButton_gsensor_start_clicked()
{
    show_gsensor_result("testing...","000000");
    this->com_write(QString("AT+GSENSOR?\r\n"));
    test_cmd_block.state=at_state_gsensor_test_wait;
    test_cmd_block.wait_time_cnt=0;
}
void mainWin::on_pb_test_qsclk_2_clicked()
{
    this->com_write(QString("AT+QSCLK=1\r\n"));
}

void mainWin::on_pb_test_qsclk_3_clicked()
{
    this->com_write(QString("AT+QSCLK=2\r\n"));
}


void mainWin::on_pushButton_gps_setting_clicked()
{
    p_gps_setting->show_param();
}

void mainWin::on_pb_imei_read_clicked()
{
    ui->lb_imei_show->setText(" ");
    this->com_write(QString("AT+EGMR=2,7\r\n"));
    test_cmd_block.state=at_state_imei_read_wait;
}
void mainWin::on_pb_ccid_read_start_clicked()
{
    test_cmd_block.state=at_state_ccid_read_send;
    test_cmd_block.send_cnt=0;
    test_cmd_block.test_enable=1;
}
bool mainWin::imei_get(QString src, QString &imei, QString &sn)
{
    src.simplified();
    src.remove(" ");
    src.remove("\r");
    src.remove("\n");
    if(src.contains(","))
    {
        QStringList  ar=src.split(",");
        if(ar.size()==2)
        {
            if(config.is_imei_infront)
            {
                imei=ar[0];
                imei=imei.simplified();
                sn=ar[1];
            }
            else
            {
                sn=ar[0];
                imei=ar[1];
                imei=imei.simplified();
            }

            if(imei.length()!=15)
            {
                if(config.is_imei_infront)
                    QMessageBox::about(NULL, "IMEI", QString("IMEI号长度错误:%1，%2\r\n当前格式：IMEI号在前").arg(imei.length()).arg(imei));
                else
                    QMessageBox::about(NULL, "IMEI", QString("IMEI号长度错误:%1，%2\r\n当前格式：IMEI号在后").arg(imei.length()).arg(imei));
                return false;
            }
        }
        else
        {
            QMessageBox::about(NULL, "IMEI", QString("格式错误\r\n应该为如下格式:\r\n012345678901234\r\nsn001,012345678901234\r\n012345678901234,sn001"));
            return false;
        }
    }
    else
    {
        if(src.length()==15)
        {
            imei=src;
            sn.clear();
        }
        else
        {
            imei.clear();
            sn.clear();
            QMessageBox::about(NULL, "IMEI", QString("IMEI号长度错误:%1").arg(src.length()));
            return false;
        }
    }
    return true;
}
void mainWin::on_pb_imei_write_clicked()
{
     QString src=ui->te_imei_write->toPlainText();
     QString t_imei,t_sn;
     if(imei_get(src,t_imei,t_sn))
     {
         if(config.is_imei_repeated_check)
         {
            bool ret=g_db.imei_exist(t_imei);
            if(ret)
            {
                ui->lb_imei_write_status->setText(QString("IMEI:%1已写过").arg(t_imei));
                return ;
            }
         }
         test_cmd_block.imeiw_input_text=src;
         test_cmd_block.imeiw_imei_text=t_imei;
         test_cmd_block.imeiw_sn_text=t_sn ;
         QString buf=QString().sprintf("AT+EGMR=1,7,\"%s\"\r\n",test_cmd_block.imeiw_imei_text.toStdString().data());
         test_cmd_block.state=at_state_imei_write_wait;
         this->com_write(buf);
         ui->lb_imei_write_status->setText("写入中...");
     }
}

void mainWin::on_te_imei_write_textChanged()
{
    QString src=ui->te_imei_write->toPlainText();
    if(src.endsWith("\n")||src.endsWith("\r"))
    {
        on_pb_imei_write_clicked();
    }
}
void mainWin::on_pb_records_output_clicked()
{
    QString dir_save = QFileDialog::getSaveFileName(this,tr("imei_records"),".",tr("XLS Files(*.xlsx)"));
    if(dir_save.isEmpty())
    {
        return ;
    }
    g_db.imei_xls_output(dir_save);
    QMessageBox::information(this,"导出excel","导出完成");
}
void mainWin::on_pb_imei_record_look_clicked()
{
    QSqlDatabase db = QSqlDatabase::database("sqlite1"); //建立数据库连接
    QSqlQuery query(db);
    query.exec("select * from imei");
    QString line;
    line=QString().sprintf("ID TIME IMEI SN RAWINPUT CCID STATUE");
    show_log(line);
    int n=1;
    while(query.next())
    {
        n++;
        line=QString("%1,%2,%3,%4,%5,%6,%7").arg(query.value(0).toString())
                                            .arg(query.value(1).toString())
                                            .arg(query.value(2).toString())
                                            .arg(query.value(3).toString())
                                            .arg(query.value(4).toString())
                                            .arg(query.value(5).toString())
                                            .arg(query.value(6).toString());
        line=line.replace(QRegExp("\r|\n")," ");
        show_log(line);
    }
}
void mainWin::on_pb_ccid_records_output_clicked()
{
    QString dir_save = QFileDialog::getSaveFileName(this,tr("ccid_records"),".",tr("XLS Files(*.xlsx)"));
    if(dir_save.isEmpty())
    {
        return ;
    }
    g_db.ccid_xls_output(dir_save);
    QMessageBox::information(this,"导出excel","导出完成");
}
void mainWin::on_pushButton_5_clicked()
{
    this->com_write(QString("AT+CPIN?\r\n"));
}



void mainWin::on_pb_http_get_start_clicked()
{
    g_http_test_on=1;
    g_http_test_cnt=1;
    ui->pb_http_get_start->setEnabled(false);
    ui->pb_http_get_stop->setEnabled(true);
}
void mainWin::on_pb_http_get_stop_clicked()
{
     g_http_test_on=0;
     ui->pb_http_get_start->setEnabled(true);
     ui->pb_http_get_stop->setEnabled(false);
}
void mainWin::on_pb_http_get_single_clicked()
{
    QString url=ui->te_http_get_url->toPlainText();
    QUrl turl(url);
    ui->te_http_get_result->clear();
    g_network_manager->get(QNetworkRequest(turl));
    show_log(QString("net open lock").arg(g_http_test_cnt),QColor("black"),true);

}

void mainWin::reply_http_get(QNetworkReply *reply)
{
    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug()<<"http reply:"<<status_code;
    //无错误返回
    if(reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();  //获取字节
        QString result(bytes);  //转化为字符串
        qDebug()<<"http reply:"<<result;
        ui->te_http_get_result->setText(QString("status:%1\r\n%2").arg(status_code).arg(result));
    }
    else
    {
         ui->te_http_get_result->setText(QString("error status:%1\r\n%2").arg(status_code));
    }
    //收到响应，因此需要处理
    delete reply;
}

void mainWin::on_pushButton_log_find_clicked()
{
    QString sf=ui->lineEdit_log_find_str->text();
    pmainlog->log_find(sf);
}


int mainWin::ota_get_txt_imei()
{
    QString src=ui->te_imei_write_text->toPlainText();
    QString t_imei,t_sn;
    if(imei_get(src,t_imei,t_sn))
    {
        test_cmd_block.imeiw_input_text=src;
        test_cmd_block.imeiw_imei_text=t_imei;
        test_cmd_block.imeiw_sn_text=t_sn ;
        return 0;
    }
    return -1;
}

void mainWin::on_pb_ota_post_start_clicked()
{
    int result = ota_get_txt_imei();
    if(0 == result){
        test_cmd_block.ota_imei_check = 1;
        test_cmd_block.ota_check = 1;
        test_cmd_block.state = at_state_imei_read_wait;
        QString buf=QString().sprintf("AT+EGMR=2,7\r\n");
        this->com_write(buf);
        ui->lb_check_ota_result->setText(QString("校验中"));
        ui->lb_check_ota_result->setStyleSheet("background-color:white");
    }else{
        ui->lb_check_ota_result->setText(QString("IMEI"));
        ui->lb_check_ota_result->setStyleSheet("background-color:red");
          //异常停止。
    }
}

void mainWin::on_pb_check_ota_start_clicked()
{
    int result = ota_get_txt_imei();
    if(0 == result){
        test_cmd_block.ota_check = 2;
        test_cmd_block.ota_imei_check = 1;
        test_cmd_block.state = at_state_imei_read_wait;
        QString buf=QString().sprintf("AT+EGMR=2,7\r\n");
        this->com_write(buf);
        ui->lb_check_ota_result->setText(QString("校验中"));
        ui->lb_check_ota_result->setStyleSheet("background-color:white");
    }else{
        ui->lb_check_ota_result->setText(QString("IMEI"));
        ui->lb_check_ota_result->setStyleSheet("background-color:red");
          //异常停止。
    }
    ui->lb_ota_post_data->setText(QString("  "));
    ui->lb_ota_post_result->setText(QString("   "));
    ui->lb_ota_post_result->setStyleSheet("background-color:white");
}

void mainWin::ota_stop()
{
//    ui->te_imei_write_text->clear();
    test_cmd_block.ota_imeir_text.clear();
    test_cmd_block.ota_imei_write = 0;
    test_cmd_block.ota_imei_check = 0;
    test_cmd_block.imeiw_imei_text.clear();
    test_cmd_block.state=at_state_idle;
}

//千寻账号设置
//千寻账号设置
//千寻账号设置
void mainWin::on_pushButton_15_clicked()
{
//    uint8_t keystr[16] = {0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0};
//    uint8_t* data_p;
//    uint8_t result[128] = {0,};
//    uint8_t out[128] = {0,};
//    ht_memset(result, 0x00,128);
//    ht_memset(out, 0x00,128);
//    aes128_block.imei = ui->textEdit_imei->toPlainText();
//    aes128_block.key = ui->textEdit_key->toPlainText();
//    aes128_block.secret = ui->textEdit_secret->toPlainText();
//    QString data_s = aes128_block.imei + aes128_block.key + aes128_block.secret;
//    int len = data_s.length();
//    QByteArray data_c = data_s.toLatin1();
//    data_p = (uint8_t*)data_c.data();

//    aes_encrypt_128_extend(keystr,data_p,result);
//    aes_decrypt_128_extend(len, keystr, result, out);
//    QString result_s((const char*)result);
//    QString out_s((const char*)out);
//    QByteArray result_c;
//    result_c = QByteArray((const char*)result);
//    QString tempDataHex = utilMan::toHexStr(QByteArray(result_c));
//    ui->textEdit_result->setText(QString(tempDataHex));
//    ui->textEdit_result_2->setText(QString(out));
}

/************************************************************************
 * @function: ValueToHexChar
 * @描述: HexCharToValue的反运算
 *
 * @参数:
 * @param: val 要转换的值.范围0-0x0f
 * @param: iscap 输出的ascii是否为大写
 *
 * @返回:
 * @return: unsigned char  0xFF:无法转换
 *         如val=11, 若iscap为真则输出'B';为假,则输出'b'.
 * @说明:
 * @作者: xugl (2013/6/6)
 *-----------------------------------------------------------------------
 * @修改人:
 ************************************************************************/
unsigned char ValueToHexChar(unsigned char val, char iscap)
{
    if (val <= 9)
    {
        val += '0';                          //0-9
    }
    else if (val >= 0x0A && val <= 0x0F)
    {
        if (iscap)
        {
            val += 'A' - 10;                //A-F
        }
        else
        {
            val += 'a' - 10;                //a-f
        }
    }
    else
    {
        val = 0xFF;                         //无效
    }

    return val;                             //返回最终结果
}
/************************************************************************
 * @function: ByteArrayBcdToHexString
 * @描述: 将BCD字节串转成ascii
 *
 * @参数:
 * @param: bcdary 需要转换的数据
 * @param: ascii 转换好的字符串
 * @param: len 需要转换的数据的长度
 * @param: big 是否big-endian输出
 * @返回:
 * @说明: 如 hexary[0] = 0x01, hexary[1] = 0x0A, len = 2,
 *       若big为真,输出ascii[0]-[3]为 '0' '1' '0' 'A'
 *       若big为假,输出ascii[0]-[3]为 '0' 'A' '0' '1'
 * @作者: xugl (2013/6/6)
 *-----------------------------------------------------------------------
 * @修改人:
 ************************************************************************/
void ByteArrayBcdToHexString(unsigned char* bcdary, unsigned char* ascii, unsigned int len, char big)
{
    for (unsigned char uc_i = 0; uc_i < len; uc_i++)
    {
        if (big)
        {
            ascii[(uc_i << 1)] = ValueToHexChar(bcdary[uc_i] >> 4, 1);
            ascii[(uc_i << 1) + 1] = ValueToHexChar(bcdary[uc_i] & 0x0F, 1);
        }
        else
        {
            ascii[((len - uc_i - 1) << 1)] = ValueToHexChar(bcdary[uc_i] >> 4, 1);
            ascii[((len - uc_i - 1) << 1) + 1] = ValueToHexChar(bcdary[uc_i] & 0x0F, 1);
        }
    }
}
/******************************************************************************************
Function            :   AT_Bytes2String
Description     :       transfer bytes to ascii string
Called By           :   ATS moudle
Data Accessed       :
Data Updated    :
Input           :   UINT8 * pSource, UINT8 * pDest, UINT8 nSourceLen
Output          :
Return          :   INT8
Others          :   build by wangqunyang 2008.05.22
*******************************************************************************************/
BOOL AT_Bytes2String(unsigned char *pDest, unsigned char *pSource, unsigned char *nSourceLen)
{

    unsigned char nTemp = 0;
    unsigned char nDestLen = 0;

    if ((NULL == pSource) || (NULL == pDest))
    {
        return FALSE;
    }

    while (nTemp < *nSourceLen)
    {
        /* get high byte */
        pDest[nDestLen] = (pSource[nTemp] >> 4) & 0x0f;

        if (pDest[nDestLen] < 10)
        {
            pDest[nDestLen] |= '0';
        }
        else
        {
            pDest[nDestLen] += 'A' - 10;
        }

        nDestLen++;

        /* get low byte */
        pDest[nDestLen] = pSource[nTemp] & 0x0f;

        if (pDest[nDestLen] < 10)
        {
            pDest[nDestLen] |= '0';
        }
        else
        {
            pDest[nDestLen] += 'A' - 10;
        }

        nDestLen++;

        nTemp++;
    }

    pDest[nDestLen] = '\0';

    *nSourceLen = nDestLen;

    /* string char counter must be even */

    if (*nSourceLen % 2)
    {
        return FALSE;
    }

    return TRUE;
}
/******************************************************************************************
Function            :   AT_String2Bytes
Description     :       This functions can transfer ascii string to bytes
Called By           :   ATS moudle
Data Accessed       :
Data Updated        :
Input           :       UINT8 * pDest, UINT8 * pSource, UINT8* pLen
Output          :
Return          :   INT8
Others          :   build by wangqunyang 2008.05.22
*******************************************************************************************/
BOOL AT_String2Bytes(unsigned char *pDest, unsigned char *pSource, unsigned char *pLen)
{
    unsigned char nSourceLen = *pLen;
    unsigned char nTemp = 0;
    unsigned char nByteNumber = 0;

    if ((NULL == pSource) || (NULL == pDest))
    {
        return FALSE;
    }

    /* string char counter must be even */
    if (nSourceLen % 2)
    {
        return FALSE;
    }

    while (nTemp < nSourceLen)
    {
        /* get high half byte */
        if ((pSource[nTemp] > 0x2f) && (pSource[nTemp] < 0x3a))
        {
            pDest[nByteNumber] = (pSource[nTemp] - '0') << 4;
        }
        else if ((pSource[nTemp] > 0x40) && (pSource[nTemp] < 0x47))
        {
            pDest[nByteNumber] = (pSource[nTemp] - 'A' + 10) << 4;
        }
        else if ((pSource[nTemp] > 0x60) && (pSource[nTemp] < 0x67))
        {
            pDest[nByteNumber] = (pSource[nTemp] - 'a' + 10) << 4;
        }
        else
        {
            return FALSE;
        }

        /* get low half byte */
        nTemp++;

        if ((pSource[nTemp] > 0x2f) && (pSource[nTemp] < 0x3a))
        {
            pDest[nByteNumber] += (pSource[nTemp] - '0');
        }
        else if ((pSource[nTemp] > 0x40) && (pSource[nTemp] < 0x47))
        {
            pDest[nByteNumber] += (pSource[nTemp] - 'A' + 10);
        }
        else if ((pSource[nTemp] > 0x60) && (pSource[nTemp] < 0x67))
        {
            pDest[nByteNumber] += (pSource[nTemp] - 'a' + 10);
        }
        else
        {
            return FALSE;
        }

        nTemp++;

        nByteNumber++;
    }

    pDest[nByteNumber] = '\0';

    *pLen = nByteNumber;

    return TRUE;
}
void mainWin::on_pushButton_16_clicked()
{
    QString filename = ui->textEdit_account_filename->toPlainText();
    QXlsx::Document xlsx(filename);
    QXlsx::Format format1;
    format1.setFontColor(QColor(Qt::red));
    QXlsx::CellRange range = xlsx.dimension();
    int rowCounts = range.lastRow();               //获取最后一行
    aes128_t aes_128;
    aes_128.key = xlsx.read(1,2).toString();
    aes_128.secret = xlsx.read(1,3).toString();
    uint8_t keystr[16] = {0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0};
    uint8_t* data_p;
    uint8_t result[128] = {0,};
    uint8_t out[256] = {0,};
    QByteArray data_c;
    QString data_s;
    uint8_t imei_row = 1;
    uint8_t reuslt_row = 2;
    unsigned char data_len = 0;
    for(int i=3;i<=rowCounts;i++)
    {
        aes_128.imei = xlsx.read(i,imei_row).toString();
        data_s = aes_128.imei + aes_128.key + aes_128.secret+"00000000000";
        data_c = data_s.toLatin1();
        data_p = (uint8_t*)data_c.data();
        aesEncrypt(keystr,16, data_p,result, 96);
//        aesDecrypt(keystr,16, result, out, 96);
        data_len = 96;
        AT_Bytes2String(out,result, &data_len);
        aes_128.result = QString(QLatin1String((char*)out));
        xlsx.write(i,reuslt_row, aes_128.result, format1);
    }
    xlsx.save();
}
//zwj  保存账号设置结果
void mainWin::save_account_set_result(QString result, QString imei)
{
    QString patch = qApp->applicationDirPath();
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString date = "/account_result" + current_date_time.toString("yyyy-MM-dd") + ".txt";
    patch.append(date);
    QFile file(patch);
    if(!file.open(QIODevice::Append|QIODevice::Text))
    {
        QMessageBox::critical(NULL,"提示","无法创建文件");
        return;
    }

    QString current_date = current_date_time.toString("yyyy/MM/dd hh:mm:ss");

    QTextStream txtOutput(&file);
    txtOutput << current_date + "***" + imei + "***" + result << endl;
    file.close();
}
void mainWin::qxwz_account_set(QString imei)
{
    uint8_t imei_row = 1;
    uint8_t reuslt_row = 2;
    char ret = -1;
//    QString imei = ui->textEdit_account_imei->toPlainText();
    QString filename = ui->textEdit_account_filename->toPlainText();
    QXlsx::Document xlsx(filename);
    QXlsx::CellRange range = xlsx.dimension();
    int rowCounts = range.lastRow();               //获取最后一行
    if(rowCounts < 0)
        ret = -2;

    aes128_t aes_128;
    for(int i=1;i<=rowCounts;i++){
        aes_128.imei = xlsx.read(i,imei_row).toString();
        if(imei == aes_128.imei){
            aes_128.result = xlsx.read(i,reuslt_row).toString();
            if(aes_128.result.length() != 192){
                ret = -3;
                break;
            }
            QString buf = "AT+ACCOUNT=\"" + aes_128.result + "\"\r\n";
            this->com_write(buf);
            ret = 0;
            break;
        }
    }
    QString showlog;
    switch(ret)
    {
        case -1:
            showlog = "IMEI未找到";
            break;
        case -2:
            showlog = "文件未找到";
            break;
        case -3:
            showlog = "加密数据长度错误";
            break;
    }
    if(ret < 0){
        ui->lb_account_result->setText(showlog);
        ui->lb_account_result->setStyleSheet("background-color:red");
        save_account_set_result(showlog, aes_128.imei);
    }
}

void mainWin::on_pushButton_17_clicked()
{
//    ui->lb_account_result->clear();
//    test_cmd_block.qxwz_account_imei.clear();
//    QString imei = ui->textEdit_account_imei->toPlainText();
//    if(imei.length() == 15){
//        qxwz_account_set(imei);
//    }else{
//        this->com_write(QString("AT+EGMR=2,7\r\n"));
//        test_cmd_block.qxwz_account_set = 1;
//        test_cmd_block.state = at_state_imei_read_wait;
//    }

}

void mainWin::on_pushButton_18_clicked()
{
    uint8_t keystr[16] = {0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0};
    aes128_t aes_128;
    unsigned char data_len = 192;
    uint8_t result[256] = {0,};
    uint8_t out[128] ={0,};
    aes_128.result = ui->textEdit_account_cipx->toPlainText();
    QByteArray ba = aes_128.result.toLatin1();
    char *mm = ba.data();
    AT_String2Bytes((uint8_t*)result,(uint8_t*)mm,&data_len);
    aesDecrypt(keystr,16, result, out, 96);
    return;
}

void mainWin::on_pushButton_17_released()
{
    ui->lb_account_result->clear();
    test_cmd_block.qxwz_account_imei.clear();
    QString imei = ui->textEdit_account_imei->toPlainText();
    if(imei.length() == 15){
        qxwz_account_set(imei);
    }else{
        this->com_write(QString("AT+EGMR=2,7\r\n"));
        test_cmd_block.qxwz_account_set = 1;
        test_cmd_block.state = at_state_imei_read_wait;
    }
}

void mainWin::on_pushButton_19_clicked()
{
    QString ctxt = ui->textEdit_account_cipx->toPlainText();
    QString buf = "AT+ACCOUNT=\"" + ctxt + "\"\r\n";
    this->com_write(buf);
}
