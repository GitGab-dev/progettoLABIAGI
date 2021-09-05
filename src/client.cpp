#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <signal.h>
using namespace std;


#define PORT 8080

pthread_t tid;
string confirm = "KO";
int T = 10; //timeout(in seconds)

string login(){
    string data;
    string temp;
    
    cout << "Welcome to the Pick and Delivery system. Please login.\n" << endl;
    cout << "Username: ";
    cin >> temp;
    data = (temp+";");
    cout << "Password: ";
    cin >> temp;
    data += (temp+";");
    cout << "\nPlease select one of the following option (selecting any other number will close the system):\n" << endl;
    cout << "1. I want to SEND a package to someone." << endl;
    cout << "2. I want to RECIEVE a package." << endl;
    cout << "\n> ";
    cin >> temp;
    cout << "\n";
    if(temp!="1" && temp!="2") return string("EXIT");
    
    if(temp=="1"){
        data+="s;";
        cout << "\n Please specify the username of the reciever: ";
        cin >> temp;
        data+=temp;
    }else data+="r";

    return data; //username;password;role
}

void* confirmation(void *arg)
{
    while(true){
        cin >> confirm;
        if(confirm == "OK"){
            cout << "Sending confirm to the server..." << endl;
            break;
        }else cout << "Write OK: ";
    }

    return nullptr;
}

void alarm_handler(int a)
{
    if(confirm != "OK"){
        cout << "\nYou took too much time!" <<endl;
        pthread_cancel(tid);    // terminate thread
    }   
}

int main(int argc, char** argv){

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    string presentation, msg;
    

    char buffer[1024] = {0};


    while(true){
        presentation = login();
        if(presentation == "EXIT"){
            cout << "You decided to exit the system. Thank you for using Pick and Delivery!" << endl;
            return 0;
        };

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            cout << "\n Socket creation error \n";
            return -1;
        }
    
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        
        // Convert IPv4 and IPv6 addresses from text to binary form
        if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
        {
            cout << "\nInvalid address/ Address not supported \n";
            return -1;
        }
    
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            cout << "\nConnection Failed \n";
            return -1;
        }

        //Sending info to server
        
        send(sock , presentation.c_str() , presentation.size() , 0 );

        valread = read( sock , buffer, 1024);
        msg = string(buffer);

        if(msg == "ERR_1"){//LOGIN FAIL
            cout << "ERROR: login failed. Check your username and password and retry.\n";
            continue;
        }else break;
    }
    std::printf("%s\n",buffer );
    

    //Communication start
    while(true){
        memset(buffer,0,1024);
        valread = read( sock , buffer, 1024);
        msg = string(buffer);
        
        if(msg.back() == '\n') msg.pop_back();
        //cout << "[" << msg << "]" << endl;

        if(msg == "CMD_1"){ //Put the package

            confirm = "KO";
            cout << "The robot arrived. Please put your package on the robot.\nWrite OK to send the package(you have 30 seconds to do so): ";
            pthread_create(&tid, nullptr, confirmation, nullptr);

            signal(SIGALRM, alarm_handler);
            alarm(T);   // Run alarm_handler after 3 seconds, and terminate thread in it

            pthread_join(tid, nullptr); // Wait for thread finish
            send(sock , confirm.c_str() , confirm.size() , 0 );
            
            
        }else if(msg == "CMD_2"){ //Take the package

            confirm = "KO";
            cout << "The robot arrived. Please take your package from the robot.\nWrite OK to send the robot back(you have 30 seconds to do so): ";
            
            pthread_create(&tid, nullptr, confirmation, nullptr);

            signal(SIGALRM, alarm_handler);
            alarm(T);   // Run alarm_handler after 3 seconds, and terminate thread in it

            pthread_join(tid, nullptr); // Wait for thread finish
            send(sock , confirm.c_str() , confirm.size() , 0 );

        }else if(msg == "CMD_EXIT"){
            cout << "The dispatch has been a success! The robot is going home now." << endl;
            break;
        }else if(msg == "MSG_ERR"){//some message had been recieved wrongly
            cout << "Something went wrong with the communication! Abort!" << endl;
            break;
        }else{
            cout << msg << endl;
        }

        
    }

    cout << "Thank you for using the Pick and Delivery system!" << endl;
    close(sock);

    return 0;
}