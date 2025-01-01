#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <cstring> // для обработки строк
#include <sys/socket.h>
#include <netinet/in.h> // Для структуры sockaddr_in и констант протоколов.
#include <unistd.h>
#include <algorithm>


std::vector<int> clients; // контейнер для хранения сокетов клиентов
std::mutex clients_mutex;

void handle_client(int client_socket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0); // получение данных от клиента

        if (bytes_received <= 0) {
            close(client_socket);
            std::lock_guard<std::mutex> lock(clients_mutex);
            std::vector<int>::iterator iter = std::remove(clients.begin(), clients.end(), client_socket);
            clients.erase(iter, clients.end());
            break;
        } // обработка ситуации, если клиент отключился или произошла ошибка и передалось 0 или меньше байтов информации

        std::cout << "Received: " << buffer << std::endl;

        // Рассылка сообщения всем клиентам
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (int client : clients) {
            if (client != client_socket) {
                send(client, buffer, bytes_received, 0);
                // отправка всем клиентам полученный текст от отдного из клиентов
            }
        }
    }
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // Создается TCP сокет
    sockaddr_in server_addr{};
    // Пустая структура с информацие о порте, типе ip, ip сервера
    server_addr.sin_family = AF_INET;
    // уст IPv4
    server_addr.sin_port = htons(8080);
    // порт 8080
    server_addr.sin_addr.s_addr = INADDR_ANY;
    // принимается подключение со всех сетеых интерфейсов

    bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    // связывает сокет с указанным адресом и портом
    listen(server_socket, 5);
    // режи ожидания входящих соединений из очереди максимальной длины 5

    std::cout << "Server started on port 8080" << std::endl;

    while (true) {
        sockaddr_in client_addr{};

        socklen_t client_len = sizeof(client_addr);
        // Пустая структура с информацие о порте, типе ip, ip клиента
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
        // возвращает сокет который будет использоваться для общения с сервером

        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_socket);

        std::thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}
