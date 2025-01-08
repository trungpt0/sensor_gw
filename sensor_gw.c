#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT 6543
#define MAX_SENSORS 10
#define FIFO_NAME "logFifo"

// sensor node
typedef struct {
	int sensorID;
	float temperature;
	char timestamp[20];
} SensorData;

// shared data
typedef struct {
	SensorData data[MAX_SENSORS];
	int count;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} SharedBuffer;

SharedBuffer sbuffer = { .count = 0, .mutex = PTHREAD_MUTEX_INITIALIZER, .cond = PTHREAD_COND_INITIALIZER };

// room sensor
typedef struct {
	int roomNumber;
	int sensorID;
} RoomSensorMap;

RoomSensorMap roomSensorMap[] = {
	{501, 10}, {602, 20}, {703, 30}, {804, 40}, {905, 50}
};

void *connectionManager(void *arg);
void *dataManager(void *arg);
void *storageManager(void *arg);
void *logProcess(void *arg);
void logEvent(const char *message);

int main()
{
	pthread_t connMgrThread, dataMgrThread, storageMgrThread, logProcThread;

	if (mkfifo("logFIFO", 0666) == -1) {
		perror("FIFO creation failed");
	}

	pthread_create(&connMgrThread, NULL, connectionManager, NULL);
	pthread_create(&dataMgrThread, NULL, dataManager, NULL);
	pthread_create(&storageMgrThread, NULL, storageManager, NULL);
	pthread_create(&logProcThread, NULL, logProcess, NULL);

	pthread_join(connMgrThread, NULL);
	pthread_join(dataMgrThread, NULL);
	pthread_join(storageMgrThread, NULL);
	pthread_join(logProcThread, NULL);

	return 0;
}

void *connectionManager(void *arg)
{
	int server_fd, new_socket;
	struct sockaddr_in sv_addr;
	int addrlen = sizeof(sv_addr);
	char buffer[256];
	
	// create socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(PORT);
	sv_addr.sin_addr.s_addr = INADDR_ANY;

	// bind socket
	if (bind(server_fd, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) == -1) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	
	// listen socket
	if (listen(server_fd, 3)) {
		perror("listen failed");
		exit(EXIT_FAILURE);
	}
	
	// accept connections and simulate sensor data
	while ((new_socket = accept(server_fd, (struct sockaddr *)&sv_addr, (socklen_t *)&addrlen)) >= 0) {
		read(new_socket, buffer, sizeof(buffer));
		int sensorID;
		float temp;
		sscanf(buffer, "%d %f", &sensorID, &temp);

		SensorData sensor = { .sensorID = sensorID, .temperature = temp };
		time_t now = time(NULL);
		strftime(sensor.timestamp, sizeof(sensor.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

		pthread_mutex_lock(&sbuffer.mutex);
		sbuffer.data[sbuffer.count++] = sensor;
		pthread_cond_signal(&sbuffer.cond);
		pthread_mutex_unlock(&sbuffer.mutex);

		logEvent("Connection Manager: Received data from sensor");
		close(new_socket);
	}
	return NULL;
}

void *dataManager(void *arg)
{
	while (1) {
		pthread_mutex_lock(&sbuffer.mutex);
		while (sbuffer.count == 0) {
			pthread_cond_wait(&sbuffer.cond, &sbuffer.mutex);
		}

		for (int i = 0; i < sbuffer.count; i++) {
			if (sbuffer.data[i].temperature > 30) {
				logEvent("Data Manager: Temperature too hot");
			} else if (sbuffer.data[i].temperature < 15.0) {
				logEvent("Data Manager: Temperature too cold");
			}
		}

		pthread_mutex_unlock(&sbuffer.mutex);
		sleep(1);	
	}
	return NULL;
}

void *storageManager(void *arg)
{
	while (1) {
		pthread_mutex_lock(&sbuffer.mutex);
		while (sbuffer.count == 0) {
			pthread_cond_wait(&sbuffer.cond, &sbuffer.mutex);
		}

		SensorData sensor = sbuffer.data[--sbuffer.count];
		pthread_mutex_unlock(&sbuffer.mutex);

		printf("Storage Manager: Storing data for sensor %d: %.2f°C at %s\n",
				sensor.sensorID, sensor.temperature, sensor.timestamp);
		logEvent("Storage Manager: Data stored");
		sleep(2);
	}
	return NULL;
}

void *logProcess(void *arg)
{
	int fifo_fd = open(FIFO_NAME, O_RDONLY);
	char buffer[256];

	while (read(fifo_fd, buffer, sizeof(buffer)) > 0) {
		FILE *logFile = fopen("gateway.log", "a");
		if (logFile) {
			fprintf(logFile, "%s\n", buffer);
			fclose(logFile);
		}
	}
	close(fifo_fd);
	return NULL;
}

void logEvent(const char *message)
{
	int fifo_fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
	if (fifo_fd != -1) {
		write(fifo_fd, message, strlen(message) + 1);
		close(fifo_fd);
	}
}
