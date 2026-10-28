#pragma once
#include "BusinessTask.h"
#include "protecol.h"
#include "FileUploadTask.h"
class TaskController
{
public:
	static BusinessTask* create_task(SERVICE_TYPE, void* data, int size, int fd);
};

