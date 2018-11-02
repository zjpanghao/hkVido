#include"opencv2/objdetect/objdetect.hpp"
#include"opencv2/highgui/highgui.hpp"  
#include"opencv2/imgproc/imgproc.hpp" 
#include<vector>
#include <list>
#include <mutex>

using namespace std;
  
using namespace cv;  
  
//人脸检测的类  

static CascadeClassifier* createClassifier() {
    CascadeClassifier *faceCascade = new CascadeClassifier();
    faceCascade->load("/home/panghao/opencv-3.4.0/data/haarcascades/haarcascade_frontalface_alt2.xml");   
    return faceCascade;
}

class FaceDetect {
 private:
  std::list<CascadeClassifier*> agentList_;
  std::mutex  lock_;
 public:

  void init() {
    for (int i = 0; i < 10; i++) {
      CascadeClassifier *faceCascade = createClassifier();
      printf("init %x\n", faceCascade);
      if (faceCascade) {
        agentList_.push_back(faceCascade);
      }
    }
  }
  
  void returnResource(CascadeClassifier* agent) {
    std::lock_guard<std::mutex> guard(lock_);
    printf("return resource\n");
    if (agent) {
      agentList_.push_back(agent);
    }
  }

  CascadeClassifier* getClassifier() {
    std::lock_guard<std::mutex> guard(lock_);
    if (agentList_.empty()) {
      return NULL;
    } else {
      CascadeClassifier *agent = agentList_.front();
      agentList_.pop_front();
      return agent;
    }
  }

};

static FaceDetect *getDetect() {
  static FaceDetect detect;
  return &detect;
}

void faceInit() {
  FaceDetect *detect = getDetect();
  detect->init();
}

class DetectWrapper {
  public:
   DetectWrapper() {
     cascade_ = getDetect()->getClassifier();
   }

   ~DetectWrapper() {
     if (cascade_) {
       getDetect()->returnResource(cascade_);     
     }
   }

   CascadeClassifier *getClassifier() {
     return cascade_;
   }

  private:
   CascadeClassifier* cascade_{NULL};
};
 
int faceNum(std::vector<unsigned char> &data)  
{  
    //加载分类器，注意文件路径  
  
    //Mat img = imread("/home/panghao/timg.jpg");  
    if (data.size() == 0) {
      return -1;
    }
    Mat img = imdecode(data, 1);
    Mat imgGray;  
    vector<Rect> faces;  
  
    if(img.empty())  
    {  
      return -1;  
    }  
  
    if(img.channels() ==3)  
    {  
       cvtColor(img, imgGray, CV_RGB2GRAY);  
    }  
    else  
    {  
       imgGray = img;  
    }  
    DetectWrapper wrapper;
    CascadeClassifier *faceCascade = wrapper.getClassifier();
    faceCascade->detectMultiScale(imgGray, faces, 1.2, 6, 0, Size(60, 60));   //检测人脸  
    int num = 0;
    if((num = faces.size())>0)  
    {  
       printf("human face detected %d\n", num);
#if 0
       for(int i =0; i<faces.size(); i++)  
       {  
           rectangle(img, Point(faces[i].x, faces[i].y), Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height),   
                           Scalar(0, 255, 0), 1, 8);    //框出人脸位置  
       }  

#endif  
    }
    return num;  
}  

