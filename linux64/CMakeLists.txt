set(base /home/panghao/kunlib)
 include_directories(include ${base}/include/turbojpg ${base}/user ${base}/user/pbase64 ${base}/include/librdkafka ${base}/user/config ${base}/include /home/panghao/kunlib/include/hiredis /home/panghao/kunlib/user/predis /home/panghao/netsdk/consoleDemo/include /usr/local/include/opencv  /usr/local/include ${base}/user/pkafka)
add_definitions( -g -std=c++11 -D__STDC_CONSTANT_MACROS)
link_directories(${base}/libs ../lib ../lib/HCNetSDKCom /home/panghao/opencv-3.4.0/build/lib /usr/local/lib)
add_executable(vide  jpg.cpp play_task.cpp sdk_init.cpp hk_api.cpp kafka_store.cpp store_factory.cpp channel.cpp store.cpp main.cpp  http-server.cpp play.cpp login.cpp dvr_control.cpp)
target_link_libraries(vide turbojpeg pkafka rdkafka kunconfig HCPreview SystemTransform hcnetsdk HCPlayBack hpr HCGeneralCfgMgr PlayCtrl  HCCore  avformat avdevice avcodec avfilter avutil swresample swscale  opencv_core -lopencv_imgcodecs opencv_imgproc hiredis event event_core glog jsoncpp pbase64  iconv z dl)
