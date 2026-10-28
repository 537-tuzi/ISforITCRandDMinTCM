#include "TaskController.h"
BusinessTask* TaskController::create_task(SERVICE_TYPE type, void* data, int size, int fd)
{
	BusinessTask* task = nullptr;
	/*if (type == SERVICE_TYPE::LOGIN) {
		task = new LoginTask(data, size, fd);
	}
	else if (type == SERVICE_TYPE::CHAT) {
		task = new ChatTask(data, size, fd);
	}*/
	cout << "type = " << type << endl;
	if (type == SERVICE_TYPE::FILE_UPLOAD) {
		task = new FileUploadTask(data,size,fd);
	}
    return task;
}
