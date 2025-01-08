#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 6543
#define BUFFER_SIZE 256
#define SERVER_IP "127.0.0.1"

void sendSensorData(int sensorID, float temp)
{
	int client_fd;
	struct sockaddr_in sv_addr;
	char buffer[BUFFER_SIZE];

	client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0) {
		perror("socket creation error");
        	exit(EXIT_FAILURE);
	}

	sv_addr.sin_family = AF_INET;
	sv_addr.sin_port = htons(SERVER_PORT);

	if (inet_pton(AF_INET, SERVER_IP, &sv_addr.sin_addr) <= 0) {
		perror("Invalid address or address not supported");
		close(client_fd);
		exit(EXIT_FAILURE);
	}

	if (connect(client_fd, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) < 0) {
		perror("Connection to the server failed");
		close(client_fd);
		exit(EXIT_FAILURE);
	}

	snprintf(buffer, sizeof(buffer), "%d %.2f", sensorID, temp);

	send(client_fd, buffer, strlen(buffer), 0);
	printf("Sensor Client: Sent data - SensorID: %d, Temperature: %.2f°C\n", sensorID, temp);

	close(client_fd);
}

int main()
{
	int sensorID = 20;
	float temperature = 21.5;

	sendSensorData(sensorID, temperature);

	return 0;
}


