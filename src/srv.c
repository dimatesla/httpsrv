#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUF_SIZE 4096



//read systemload
int readload(char *buffer, size_t bufsize){
	char path[64] = "/proc/loadavg";
	float load1, load5, load15;
	int running, total, last_proc;
	char proc[16];

	FILE *fp = fopen(path,"r");
	if(!fp){
		snprintf(buffer,bufsize,"Cant open file %s: %s\n",path, strerror(errno));
		return 1;	
	}
	//Парсим фулл файл 
	if(fscanf(fp, "%f %f %f %15s %d", &load1, &load5, &load15, proc, &last_proc) !=5 ){
		snprintf(buffer,bufsize, "Cant read file %s: %s\n",path, strerror(errno));
		fclose(fp);
		return 1;
	}
	fclose(fp);
	//Парсим отдельно процессы
	if(sscanf(proc, "%d/%d", &running, &total) !=2){
		snprintf(buffer,bufsize, "Cant do parsing proc string '%s'\n",proc);
		return 1;
	}	
	

//заполняем буфер
	snprintf(buffer,bufsize,
				"System load: \n"
				" - 1 min load: %.2f\n"
				" - 5 min load: %.2f\n"
				" - 15 min load: %.2f\n"
				" - runnable processes: %d of %d\n"
				" - last PID: %d\n",
				load1,load5,load15, running, total, last_proc);
	return 0;
}

//read temp
int readcpu(int zone_num,char *buffer, size_t bufsize){
	char path_temp[64];
	char path_type[64];
	char type[64];
	float temp_c;
	int temp;
	
	//Формирование пути 
	snprintf(path_type, sizeof(path_type), "/sys/class/thermal/thermal_zone%d/type", zone_num);
	snprintf(path_temp, sizeof(path_temp), "/sys/class/thermal/thermal_zone%d/temp", zone_num);

	FILE *fp_type = fopen(path_type,"r");
	if(!fp_type){
		snprintf(buffer,bufsize,"Ошибка открытия файла %s: %s\n",path_type,strerror(errno));
		return 1;
	}
	if(!fgets(type, sizeof(type), fp_type)){
		snprintf(buffer,bufsize,"Cant fget ftype %s: %s\n",path_type,strerror(errno));
		fclose(fp_type);
		return 1;
	}
	//Красивый вывод
	type[strcspn(type, "\n")] = '\0';
	fclose(fp_type);

	FILE *fp_temp = fopen(path_temp, "r");
	if(!fp_temp){
		snprintf(buffer,bufsize,"Ошибка открытия файла %s: %s\n",path_temp,strerror(errno));
		return 1;
	}
	if(fscanf(fp_temp, "%d", &temp) != 1){
		snprintf(buffer,bufsize,"Cannot read temp from %s: %s\n", path_temp, strerror(errno));
		fclose(fp_temp);
		return 1;
	}
	fclose(fp_temp);

	temp_c = temp/1000.0;
	//заполняем буфер
	snprintf(buffer, bufsize, "Zone %d (%s): %.3f\n", zone_num, type, temp_c);
	return 0;
}
//Пишем логи в файл
int log_write(const char *buffer){
	FILE *log = fopen("/home/orangepi/httpsrv/httpsrv/log","a");
        if(log==NULL){
        	perror("fopen");
		return -1;
        }
        if(fprintf(log, "Client sent: \n%s\n", buffer)<0){
               	perror("fprintf");
		fclose(log);
		return -2;
	}
        if(fclose(log)!= 0){
        	perror("fclose");
		return -3;
        }
	return 0;
}


int main(){
	int server_fd, client_fd;
	struct sockaddr_in addr;
	char buffer[BUF_SIZE];
	char info0[256], info1[256], info2[512];
	char http_response[2048];
	char http_body[1024];

//Создание сокета 
	server_fd = socket(AF_INET, SOCK_STREAM,0);
	if(server_fd == -1){
		//printf("socket failed: %d\n", errno);
		perror("socket");
		exit(1);
	}

//Настройка адреса 
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT);
//Ошибка 98, порт занят
	int opt = 1;
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
		perror("setsockport");
		close(server_fd);
		exit(1);
	}

	
//Привязка 
	if(bind(server_fd,(struct sockaddr*)&addr, sizeof(addr))<0){
		printf("bind failed: %d\n", errno);
		exit(1);		
	}
//Слушаем 
	if(listen(server_fd,5)<0){
		perror("listen error: ");
		exit(1);
	}

	printf("Server started on PORT: %d...\n", PORT);



	while(1){

//Проверяем на соединение парсим сообщениe
//
//
		client_fd = accept(server_fd, NULL, NULL);
		if(client_fd < 0){
			perror("accept failed");
			continue;
		}
		int bytes_read = read(client_fd,buffer,BUF_SIZE - 1);
		if(bytes_read > 0){
			buffer[bytes_read] = '\0';
		}

//Пишем в лог файл
		int result = log_write(buffer);
		if(result!=0){
			printf("failed code: %d\n", result);
		}
//Читаем нагрузку на процессор and temp
		readcpu(0, info0, sizeof(info0));
		readcpu(1, info1, sizeof(info1));
		readload(info2, sizeof(info2));
//Формируем тело ответа 
		snprintf(http_body, sizeof(http_body), "%s%s%s", info0, info1,info2);
//Формируем полный http пакет
		snprintf(http_response, sizeof(http_response),
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: %zu\r\n"
				"Connection: close\r\n"
				"\r\n"
				"%s", strlen(http_body),http_body);
//Send
		send(client_fd, http_response, strlen(http_response),0);
        	close(client_fd);
	}	
	return 0;
}
