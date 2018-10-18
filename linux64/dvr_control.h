#ifndef INCLUDE_DVR_CONTROL_H
#define INCLUDE_DVR_CONTROL_H
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include "HCNetSDK.h"
#include "play.h"
//#include "thread_pool.h"
#include "sdk_common.h"
#include "play_task.h"
class DVRControl {
  public:
    bool addTask(std::shared_ptr<PlayTask> playTask);
    int play(int taskId);
    int stopPlayTask(int taskId);
    int playControl(int taskId, int flag, long param);
    std::string getTaskInfo();

    void addNbytes(int taskId, long nbyte);

    int getTaskStatus(int taskId);

    int getPlayDePort(int taskId);

    void setPlayHandle(int taskId, int handle);

    int getPos(int taskId);
    
    int heartBeat(int taskId);
    void taskHeartCheck();

    bool hasFilePlay(int lUserId, int channel, long startTime, long endTime);

	void addPackToTask(const TaskParam &param);

	std::shared_ptr<PlayTask> getByTaskId(int taskId);

	static void sendThd(void *param);
	
  private:
    std::shared_ptr<PlayTask> getPlayTask(int taskId);
	
    std::map<int, std::shared_ptr<PlayTask> > playTaskMap;
    std::mutex lock;
    static constexpr int TASK_TIMEOUT_SECONDS{120 *1};
    
};

DVRControl & getDVRControl();

void heartBeatCheck();

#endif
