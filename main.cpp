#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <vector>
#include <thread>
#include <pthread.h>
#include <mutex>
#include <map>
#include <sstream>
#include <vector>
#include <regex>
#include <math.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <fstream>

using namespace std;

string useAs, myIP = "0.0.0.0", ballOn = "", serialPort, serialPortCustom;
bool ball = false, transpose = false, processing = false;
int listening, bufSize = 4096, stat=0;
vector <int> PosXYZ {0, 0, 0}, tempPosXYZ {0, 0, 0}, shift {15, 15, 15};
map<string, thread> gotoDict, th_Receiveds;
map<string, int> socketDict;
sockaddr_in client;
mutex m;
FILE *connectArduinoW;

inline bool isBlank(const string &s)
{
    return std::all_of(s.cbegin(), s.cend(), [](char c) { return std::isspace(c); });
}

string trim_control(const string &str)
{
    string s = "";
    for (auto c : str)
        if (!iscntrl(c))
            s += c;
    return s;
}

string trim_left(const string &str)
{
    const string pattern = " \f\n\r\t\v";
    return str.substr(str.find_first_not_of(pattern));
}

string trim_right(const string &str)
{
    const string pattern = " \f\n\r\t\v";
    return str.substr(0, str.find_last_not_of(pattern) + 1);
}

string trim(const string &str)
{
    return trim_control(trim_left(trim_right(str)));
}

string toLowers(string &s)
{
    for (int i =0; i< s.size(); i++)
        s[i] = tolower(s[i], locale());
    return s;
}

void swap(string &a, string &b)
{
    auto _temp = a;
    a = b;
    b = _temp;
}

void swap(int &a, int &b)
{
    auto _temp = a;
    a = b;
    b = _temp;
}

void split(string &text, char delimiter, vector<string> &vec)
{
    string item;
    for (stringstream ss(text); getline(ss, item, delimiter); (vec.push_back(item)));
}

void split(string &text, char delimiter, vector<int> &vec)
{
    string item;
    for (stringstream ss(text); getline(ss, item, delimiter); (vec.push_back(stoi(item))));
}

template<typename K, typename V>
string keyByValue(map<K, V> m, V value)
{
    for (auto &i : m)
        if (i.second == value)
            return i.first;
    return 0;
}

int kbhit(void)
{
    struct termios oldt, newt;
    int ch, oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1; }
    return 0;
}

string getMyIP(){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    int ctr = 0;
    myIP.clear();

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (trim(ifa->ifa_name) != "lo") {
                if (ctr > 0)
                    myIP += '|';
            myIP += addressBuffer; ctr++; }
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            // printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return myIP;
}

int connectArduino()
{
    // for (int i = 0; (connectArduinoW == NULL) && (i < 50); i++)
    //    connectArduinoW = fopen((serialPort = "/dev/ttyUSB" + to_string(i)).c_str(), "wr"); //Opening device file;
    // for (int i = 0; (connectArduinoW == NULL) && (i < 50); i++)
    //    connectArduinoW = fopen((serialPort = "/dev/ttyACM" + to_string(i)).c_str(), "wr"); //Opening device file;

    connectArduinoW = fopen((serialPort = "/dev/tty"+ serialPortCustom).c_str(), "wr"); //Opening device file;

    if (connectArduinoW != NULL)
        printf("[OK]  %s \n", serialPort.c_str());
    else
        printf("[X]  ERROR_CONNECT_ARDUINO ON %s \n", serialPort.c_str());
}

void setSerialport()
{
    printf("Serial Port: /dev/tty"); cin >> serialPortCustom;
    serialPortCustom = trim(serialPortCustom);
    if (regex_match(serialPortCustom.begin(), serialPortCustom.end(), regex("^(USB|usb|U|u)[0-9]{1,3}$"))) serialPortCustom = "USB" + string(1, serialPortCustom[serialPortCustom.size()-1]); 
    else if (regex_match(serialPortCustom.begin(), serialPortCustom.end(), regex("^(ACM|acm|A|a)[0-9]{1,3}$"))) serialPortCustom = "ACM" + string(1, serialPortCustom[serialPortCustom.size()-1]); 
    else serialPortCustom.clear(); 
    connectArduino();
}

void checkConnection()
{
    try {
        // for (m.lock(); true; m.unlock())
        {
            for (auto &i : socketDict) {
                string key = i.first;
                if (i.second == 0) {
                    // close(i.second);
                    th_Receiveds[key].detach();
                    th_Receiveds.erase(key);
                    socketDict.erase(key); } }
        // usleep(1000000);
        }
    }
    catch (exception e) {
        cout << "# Check Connection error \n~\n" << e.what() << endl; }
}

void sendCallBack(int clientSocket, string message)
{
    try {
        if ((!isBlank(trim(message))) /*&& (toLowers(message) != "quit")*/) {
            char buf[bufSize];

            if (clientSocket != 0) {
                //	Send to server
                int sendRes = send(clientSocket, trim(message).c_str(), message.size() + 1, 0);
                if (sendRes == -1) {
                    cout << "Could not send to server! Whoops!\r\n";
                    clientSocket = 0;
                    return; }
                else
                    if (message != ".")
                        cout << "@ " << keyByValue(socketDict, clientSocket) << " : " << trim(message) << endl; }
            else
                printf("!...Not Connected...! \n"); } }
    catch (exception e) {
        cout << "# Send Callback error \n~\n" << e.what() << endl; }
}

void sendPosXYZ()
{
    if (socketDict.count("BaseStation")) {
        string message = "E" + to_string(PosXYZ[0]) + "," + to_string(PosXYZ[1]) + "," + to_string(PosXYZ[2]);
        sendCallBack(socketDict["BaseStation"], message); }
}

void toArduino(string message)
{
    if (!isBlank(message))
        if (connectArduinoW == NULL)
            connectArduino();
        else 
        {
        // 	if((message == "x+") && (PosXYZ[0]) == tempPosXYZ[0]){
        //  		message = "r";
    			 // tempPosXYZ[0] += shift[0];
        //  	}
        // 	else if((message == "x-") && (PosXYZ[0]) == tempPosXYZ[0]){
        //  		message = "l";
    			 // tempPosXYZ[0] -= shift[0];
        //  	}
        //  	else if((message == "y+") && (PosXYZ[1]) == tempPosXYZ[1]){
        //  		message = "u";
    			 // tempPosXYZ[1] += shift[1];
        //  	}
        //  	else if((message == "y-") && (PosXYZ[1]) == tempPosXYZ[1]){
        //  		message = "d";
    			 // tempPosXYZ[1] -= shift[1];
        //  	}
        //  	else if((message == "z+") && (PosXYZ[2]) == tempPosXYZ[2]){
        //  		message = "x";
    			 // tempPosXYZ[2] += shift[2];
        //  	}
        //  	else if((message == "z-") && (PosXYZ[2]) == tempPosXYZ[2]){
        //  		message = "z";
    			 // tempPosXYZ[2] -= shift[2];
        //  	}
             //else
                 //return;
            if(stat==0)
            {
                stat=1;
                fprintf(connectArduinoW, "%s", (message + "\n").c_str());    //Writing to the Arduino
                printf("@@ Arduino : %s \n", message.c_str()); 
            }
        }
}

// void GotoLoc (string Robot, int endX, int endY, int endAngle, int shiftX, int shiftY, int shiftAngle)
// {
//     try {
//         cout << "# " << Robot << " : Goto >> " << "X:" << endX << " Y:" << endY << " ∠:" << endAngle << "°" << endl;
//         bool chk[] = {true, true, true};
//         while (chk[0] |= chk[1] |= chk[2]) {
//             if (PosXYZ[0] > 12000)
//                 PosXYZ[0] = stoi (to_string(PosXYZ[0]).substr (0, 4));
//             if (PosXYZ[1] > 9000)
//                 PosXYZ[1] = stoi (to_string(PosXYZ[1]).substr (0, 4));
//             if (PosXYZ[2] > 360)
//                 PosXYZ[2] = stoi (to_string(PosXYZ[2]).substr (0, 2));

//             if ((PosXYZ[0] > endX) && (shiftX > 0))
//                 shiftX *= -1;
//             else if ((PosXYZ[0] < endX) && (shiftX < 0))
//                 shiftX *= -1;
//             if ((PosXYZ[1] > endY) && (shiftY > 0))
//                 shiftY *= -1;
//             else if ((PosXYZ[1] < endY) && (shiftY < 0))
//                 shiftY *= -1;
//             if ((PosXYZ[2] > endAngle) && (shiftAngle > 0))
//                 shiftAngle *= -1;
//             else if ((PosXYZ[2] < endAngle) && (shiftAngle < 0))
//                 shiftAngle *= -1;

//             if (PosXYZ[0] != endX) {
//                 if (abs (endX - PosXYZ[0]) < abs (shiftX)) // Shift not corresponding
//                     shiftX = (endX - PosXYZ[0]);
//                 PosXYZ[0] += shiftX; // On process
//             } else
//                 chk[0] = false; // Done
//             if (PosXYZ[1] != endY) {
//                 if (abs (endY - PosXYZ[1]) < abs (shiftY)) // Shift not corresponding
//                     shiftY = (endY - PosXYZ[1]);
//                 PosXYZ[1] += shiftY; // On process
//             } else
//                 chk[1] = false; // DonekeyName
//             if (PosXYZ[2] != endAngle) {
//                 if (abs (endAngle - PosXYZ[2]) < abs (shiftAngle)) // Shift not corresponding
//                     shiftAngle = (endAngle - PosXYZ[2]);
//                 PosXYZ[2] += shiftAngle; // On process
//             } else
//                 chk[2] = false; // Done

//             sendPosXYZ();
//             usleep (100000); // time per limit (microsecond)
//         }
//     } catch (exception e)  {
//         cout << "% Error GotoLoc \n\n" << e.what() << endl; }
// }

void GotoLoc (string Robot, int endX, int endY, int endAngle)
{
    try {
        processing = true;
        cout << "# " << Robot << " : Goto >> " << "X:" << endX << " Y:" << endY << " ∠:" << endAngle << "°" << endl;
        string x = "x", y = "y", z = "z", direction[3];
        int timeWait = 30000;   // time per limit (microsecond)
        if (transpose == true)
            swap(x, y);
        bool chk[] = {true, true, true};
        while ((gotoDict.count(Robot)) && (chk[0] |= chk[1] |= chk[2])) {

            if (PosXYZ[0] > endX)
                direction[0] = x + "-";     //LEFT
            else if (PosXYZ[0] < endX)
                direction[0] = x + "+";     //RIGHT
            if (PosXYZ[1] > endY)
                direction[1] = y + "-";     //UP
            else if (PosXYZ[1] < endY)
                direction[1] = y + "+";     //DOWN
            if (PosXYZ[2] > endAngle)
                direction[2] = z + "-";     //AntiClockwise
            else if (PosXYZ[2] < endAngle)
                direction[2] = z + "+";     //Clockwise

            if (PosXYZ[0] != endX) {
                toArduino(direction[0]); // On process
                usleep(timeWait); }      // time per limit (microsecond)
            else
                chk[0] = false; // Done
            if (PosXYZ[1] != endY) {
                toArduino(direction[1]); // On process
                usleep(timeWait); }      // time per limit (microsecond)
            else
                chk[1] = false; // Done
            if (PosXYZ[2] != endAngle) {
                toArduino(direction[2]); // On process
                usleep(timeWait); }      // time per limit (microsecond)
            else
                chk[2] = false; // Done

            // usleep (100000); // time per limit (microsecond)
        }
        processing = false;
    } catch (exception e)  {
        cout << "% Error GotoLoc \n\n" << e.what() << endl; }
}

bool stopThread(map<string, thread> &threadMap, string keyName)
{
    if (threadMap.count(keyName)) {
        threadMap[keyName].detach();
        threadMap.erase(keyName);
        printf("# Thread key %s is STOPPED :<\n", keyName.c_str());
        return 1; }
    return 0;
}

void threadGoto (string keyName, string message)
{
    vector<int> dtXYZ;
    stopThread(gotoDict, keyName);  ///Turn it off if threre is still something running
    split(message, ',', dtXYZ);
    while(processing);
    // gotoDict[keyName] = thread(GotoLoc, useAs, dtXYZ[0], dtXYZ[1], dtXYZ[2], 20, 20, 1);     ///For NOT connected Arduino
    gotoDict[keyName] = thread(GotoLoc, useAs, dtXYZ[0], dtXYZ[1], dtXYZ[2]);       ///For connected Arduino
}

string ResponeSendCallback(int clientSocket, string message)
{
    string respone = "", text = "";
    vector<string> _dtMessage;
    split(message, '|', _dtMessage);

    if ((_dtMessage[0].find("!") == 0) && (_dtMessage[0].size() > 1)) {
        _dtMessage.clear();
        _dtMessage.push_back(_dtMessage[0].substr(1)); _dtMessage.push_back("Robot1,Robot2,Robot3"); }
    if ((_dtMessage[0].find("**") == 0) && (_dtMessage[0].size() > 2)) {
        respone = _dtMessage[0];
        goto broadcast; }
    else if ((_dtMessage[0].find("*") == 0) && (_dtMessage[0].size() > 1)) {
        goto multicast; }

    if (toLowers(_dtMessage[0]) == "myip") {
        text = "MyIP: "+ getMyIP();
        respone = getMyIP();
        if (clientSocket != 0)
            goto multicast;
        goto end; }
    else if (toLowers(_dtMessage[0]) == "as") {
        text = "UseAs: "+ useAs;
        goto end; }
    // else if ((_socketDict.ContainsKey("BaseStation")) && (socket.Client.RemoteEndPoint.ToString().Contains(_socketDict["BaseStation"].Client.RemoteEndPoint.ToString())))
    else if (clientSocket != 0)
    {
        // If to send Base Station socket
        /// LOCATION ///
        if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^(go|Go|gO|GO)[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5}$")))
        { //Goto Location
            if (_dtMessage.size() > 1)
                goto multicast;
            else
                threadGoto(useAs, _dtMessage[0].substr(2));
            goto end;
        }

        /// INFORMATION ///
        else if (_dtMessage[0] == "B")
        { //Get the Ball
            ball = true;
            respone = "B_";
            goto broadcast;
        }
        else if (_dtMessage[0] == "b")
        { //Lose the Ball
            ball = false;
            respone = "b_";
            goto broadcast;
        }

        /// COMMAND ///
        else if (_dtMessage[0] == "~")  ///Tilde character
        { //Force stop sthread
            stopThread(gotoDict, useAs);  ///Turn it off if threre is still something running
        }

        /// OTHERS ///
        else if (_dtMessage[0] == ";")
        { //PING
            respone = "ping";
            goto multicast;
        }
    }
    goto multicast;

broadcast:
    sendCallBack(clientSocket, respone + "|" + "Robot1,Robot2,Robot3");
    // sendByHostList("BaseStation", respone + "|" + "Robot1,Robot2,Robot3");
    goto end;

multicast:
    if (isBlank(respone))
        respone = _dtMessage[0];
    if (_dtMessage.size() > 1)
        sendCallBack(clientSocket, respone + "|" + _dtMessage[1]);
    else
        sendCallBack(clientSocket, respone);
    // sendByHostList("BaseStation", respone + "|" + chkRobotCollect);
    goto end;

end:
    if (!isBlank(text))
        cout << "# " << text << endl;
    return respone;
}

string ResponeReceivedCallback(int clientSocket, string message)
{
    string respone = "", text = "";
    vector<string> _dtMessage, msgXYZs, msgXYZ;
    split(message, '|', _dtMessage);
    if ((_dtMessage[0].find("!") == 0) && (_dtMessage[0].size() > 1)) {         // Broadcast message
        _dtMessage.clear();
        _dtMessage.push_back(_dtMessage[0].substr(1)); _dtMessage.push_back("Robot1,Robot2,Robot3"); }
    if ((_dtMessage[0].find("**") == 0) && (_dtMessage[0].size() > 2)) {        // Forward & Broadcast message
        respone = _dtMessage[0].substr(2);
        goto broadcast; }
    else if ((_dtMessage[0].find("*") == 0) && (_dtMessage[0].size() > 1)) {    // Forward & Multicast essage
        respone = _dtMessage[0].substr(1);
        goto multicast; }

    if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^E[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5}$")))
    {
        // If message is data X, Y, Z
        toArduino(_dtMessage[0]);

        // split(_dtMessage[0], 'E', msgXYZs);
        // split(msgXYZs.back(), ',', msgXYZ);
        // for (int i = 0; i < msgXYZ.size(); i++)
        //     PosXYZ[i] = stoi(msgXYZ[i]);
        // sendPosXYZ();
        // text = "X:" + to_string(PosXYZ[0]) + " Y:" + to_string(PosXYZ[1]) + " ∠:" + to_string(PosXYZ[2]) + "°";
    }
    else if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^(go|Go|gO|GO)[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5}$")))
    { //Goto Location
        if (_dtMessage.size() > 1)
            goto multicast;
        else
            threadGoto(useAs, _dtMessage[0].substr(2));
        goto end;
    }
    // else if ((_socketDict.ContainsKey("BaseStation")) && (socket.Client.RemoteEndPoint.ToString().Contains(_socketDict["BaseStation"].Client.RemoteEndPoint.ToString())))
    // else if ((clientSocket != 0) && (socketDict.count("BaseStation")) && (keyByValue(socketDict, clientSocket) == "BaseStation"))
    else if (clientSocket != 0)
    // else if (true)
    {
        // If socket is Base Station socket
        ////    REFEREE BOX COMMANDSt	////
        ///{
        /// 1. DEFAULT COMMANDS ///
            if (_dtMessage[0] == "S") { //STOP
                text = "STOP";
                toArduino(_dtMessage[0]); }
            else if (_dtMessage[0] == "s") { //START
                text = "START";
                toArduino(_dtMessage[0]); }
            else if (_dtMessage[0] == "W") { //WELCOME (welcome message)
                text = "WELCOME"; }
            else if (_dtMessage[0] == "Z") { //RESET (Reset Game)
                text = "RESET"; }
            else if (_dtMessage[0] == "U") { //TESTMODE_ON (TestMode On)
                text = "TESTMODE_ON"; }
            else if (_dtMessage[0] == "u") { //TESTMODE_OFF (TestMode Off)
                text = "TESTMODE_OFF"; }

        /// 3. GAME FLOW COMMANDS ///
            else if (_dtMessage[0] == "1") { //FIRST_HALF
                text = "FIRST_HALF"; }
            else if (_dtMessage[0] == "2") { //SECOND_HALF
                text = "SECOND_HALF"; }
            else if (_dtMessage[0] == "3") { //FIRST_HALF_OVERTIME
                text = "FIRST_HALF_OVERTIME"; }
            else if (_dtMessage[0] == "4") { //SECOND_HALF_OVERTIME
                text = "SECOND_HALF_OVERTIME"; }
            else if (_dtMessage[0] == "h") { //HALF_TIME
                text = "HALF_TIME"; }
            else if (_dtMessage[0] == "e") { //END_GAME (ends 2nd part, may go into overtime)
                text = "END_GAME"; }
            else if (_dtMessage[0] == "z") { //GAMEOVER (Game Over)
                text = "GAMEOVER"; }
            else if (_dtMessage[0] == "L") { //PARKING
                text = "PARKING"; }
            else if (_dtMessage[0] == "N") { //DROP_BALL
                text = "DROP_BALL"; }

        /// 2. PENALTY COMMANDS ///
            else if (_dtMessage[0] == "Y") { //YELLOW_CARD_CYAN
                text = "YELLOW_CARD_CYAN"; }
            else if (_dtMessage[0] == "R") { //RED_CARD_CYAN
                text = "RED_CARD_CYAN"; }
            else if (_dtMessage[0] == "B") { //DOUBLE_YELLOW_CYAN
                text = "DOUBLE_YELLOW_CYAN"; }

        /// 4. GOAL STATUS ///
            else if (_dtMessage[0] == "A") { //GOAL_CYAN
                text = "GOAL_CYAN"; }
            else if (_dtMessage[0] == "D") { //SUBGOAL_CYAN
                text = "SUBGOAL_CYAN"; }

        /// 5. GAME FLOW COMMANDS ///
            else if (_dtMessage[0] == "K") { //KICKOFF_CYAN
                text = "KICKOFF_CYAN"; }
            else if (_dtMessage[0] == "F") { //FREEKICK_CYAN
                text = "FREEKICK_CYAN"; }
            else if (_dtMessage[0] == "G") { //GOALKICK_CYAN
                text = "GOALKICK_CYAN"; }
            else if (_dtMessage[0] == "T") { //THROWN_CYAN
                text = "THROWN_CYAN"; }
            else if (_dtMessage[0] == "C") { //CORNER_CYAN
                text = "CORNER_CYAN"; }
            else if (_dtMessage[0] == "P") { //PENALTY_CYAN
                text = "PENALTY_CYAN"; }
            else if (_dtMessage[0] == "O") { //REPAIR_CYAN
                text = "REPAIR_CYAN"; }

        /// 2. PENALTY COMMANDS ///
            else if (_dtMessage[0] == "y") { //YELLOW_CARD_MAGENTA
                text = "YELLOW_CARD_MAGENTA"; }
            else if (_dtMessage[0] == "r") { //RED_CARD_MAGENTA
                text = "RED_CARD_MAGENTA"; }
            else if (_dtMessage[0] == "b") { //DOUBLE_YELLOW_MAGENTA
                text = "DOUBLE_YELLOW_MAGENTA"; }

        /// 4. GOAL STATUS ///
            else if (_dtMessage[0] == "a") { //GOAL_MAGENTA
                text = "GOAL_MAGENTA"; }
            else if (_dtMessage[0] == "d") { //SUBGOAL_MAGENTA
                text = "SUBGOAL_MAGENTA"; }

        /// 5. GAME FLOW COMMANDS ///
            else if (_dtMessage[0] == "k") { //KICKOFF_MAGENTA
                text = "KICKOFF_MAGENTA"; }
            else if (_dtMessage[0] == "f") { //FREEKICK_MAGENTA
                text = "FREEKICK_MAGENTA"; }
            else if (_dtMessage[0] == "g") { //GOALKICK_MAGENTA
                text = "GOALKICK_MAGENTA"; }
            else if (_dtMessage[0] == "t") { //THROWN_MAGENTA
                text = "THROWN_MAGENTA"; }
            else if (_dtMessage[0] == "c") { //CORNER_MAGENTA
                text = "CORNER_MAGENTA"; }
            else if (_dtMessage[0] == "p") { //PENALTY_MAGENTA
                text = "PENALTY_MAGENTA"; }
            else if (_dtMessage[0] == "o") { //REPAIR_MAGENTA
                text = "REPAIR_MAGENTA"; }
        ///}

        /// INFORMATION ///
        else if (_dtMessage[0].find("B_Robot") == 0)
        { //Get the ball
            text = "Ball on " + (ballOn = _dtMessage[0].substr(2));
            if (ballOn == useAs)
                ball = true;
            else
                ball = false;
        }
        else if (_dtMessage[0] == "b_")
        { //Lose the ball
            ballOn.clear();
            text = "Lose the ball  ";
        }
        else if (_dtMessage[0] == "B?")
        { //Ball Status
            if (ball)
                respone = "B_";
            else if ((!ball) && (isBlank(ballOn)))
                respone = "b_";
            else {
                respone = "0";
                goto multicast; }
            goto broadcast;
        }

        /// COMMAND ///
        else if (regex_match(_dtMessage[0].begin(), _dtMessage[0].end(), regex("^(x[+]|x-|y[+]|y-|z[+]|z-)$")))
        { //Go by Arrow
            toArduino(_dtMessage[0]);
        }
        else if (_dtMessage[0] == "~")  ///Tilde character
        { //Force stop sthread
            stopThread(gotoDict, useAs);  ///Turn it off if threre is still something running
        }

        /// OTHERS ///
        else if ((toLowers(_dtMessage[0]) == "basestation") ^ (toLowers(_dtMessage[0]) == "bs"))
        { //Info BS
            text = "BS";
            socketDict["BaseStation"] = clientSocket;
            for (auto &i : socketDict)
                if (i.first != "BaseStation") {
                    // close(i.second);
                    socketDict[i.first] = 0;
                }
            checkConnection();
        }
        else if (toLowers(_dtMessage[0]) == "ping")
        { //PING-REPLY
            respone = "Reply " + useAs;
            goto multicast;
        }
        else if (toLowers(_dtMessage[0]) == "ip")
        { //IP Address Info
            respone = getMyIP();
            goto multicast;
        }
        else if (toLowers(_dtMessage[0]) == "os")
        { //Outside
            text = "Outside";
        }
        else if (toLowers(_dtMessage[0]) == "F10")
        { //  Stat Comunication Arduino
            stat = 0;
        }
        else if (toLowers(_dtMessage[0]) == "get_time")
        { //TIME NOW
            time_t ct = time(0);
            respone = ctime(&ct);
            goto multicast;
        }
        // else
        //     respone = text = "# Invalid Command :<";
    }
    if ((isBlank(respone)) && (_dtMessage.size() > 1))
        sendCallBack(clientSocket, _dtMessage[0] + "|" + _dtMessage[1]);
    goto end;

broadcast:
    sendCallBack(clientSocket, respone + "|" + "Robot1,Robot2,Robot3");
    // sendByHostList("BaseStation", respone + "|" + "Robot1,Robot2,Robot3");
    goto end;

multicast:
    if (_dtMessage.size() > 1)
        sendCallBack(clientSocket, respone + "|" + _dtMessage[1]);
    else
        sendCallBack(clientSocket, respone);
    // sendByHostList("BaseStation", respone + "|" + chkRobotCollect);
    goto end;

end:
    if (!isBlank(text))
        cout << "# " << text << endl;
   return respone;
}

void receivedCallBack(int clientSocket)
{
    try {
        string message = "";
        // for (m.lock(); (true) && (toLowers(message) != "quit"); message.clear(), m.unlock())
        while ((true) && (toLowers(message) != "quit")) {
            /// While loop: accept and echo message back to client user Input
            char buf[bufSize];
            memset(buf, 0, bufSize);
            /// Wait for client to send data
            int bytesReceived = recv(clientSocket, buf, bufSize, 0);
            if (bytesReceived == -1) {
                sendCallBack(clientSocket, "quit");
                cerr << "Error in receivedCallBack(). Quitting" << endl;
                break; }

            if ((bytesReceived == 0) || (clientSocket == 0)) {
                sendCallBack(clientSocket, "quit");
                cout << "Client disconnected " << endl;
                break; }
            message = trim(string(buf, 0, bytesReceived));

            if (!isBlank(message)  && (message.find(".") == 0))     //time Relay
                sendCallBack(clientSocket, ".");
            else if (!isBlank(message)) {
                if (message != ".")
                    cout << "> " << keyByValue(socketDict, clientSocket) << " : " << message << endl;
                ResponeReceivedCallback(clientSocket, message); }

            /// Echo message back to client
            // send(clientSocket, buf, bytesReceived + 1, 0);
        }
        /// Close the socket
        close(clientSocket);
        clientSocket = 0;
    }
    catch (exception e) {
        cout << "# Received error \n~\n" << e.what() << endl; }
}

void startAgain()
{
	cout << "wait_start_again" << endl;
	usleep(1000000);	//Delay 1 seconds
	toArduino("s");
	toArduino("s");
	sendCallBack(socketDict["BaseStation"], "start_again");
}

void fromArduino()
{
    char buffer[1024];
    FILE *connectArduinoR = fopen((serialPort = "/dev/tty"+ serialPortCustom).c_str(), "r");

    if (connectArduinoR == NULL) {
        printf("[X]  ERROR_CONNECT_ARDUINO ON %s \n", serialPort.c_str());
        return; }

    // for (m.lock(); true; m.unlock())
    while ((true) && (connectArduinoR != NULL)) {
        memset(buffer, 0, 1024);
        fread(buffer, sizeof(char), 1024, connectArduinoR);
        string message = string(buffer);
        if (!isBlank(message)) {
            message = trim(message);
            cout << message << endl;    // Print All Message, No Filter
            stat=0;

            if(message.find("s") <= message.size())
            {
            	cout << "get_s" << endl;
            	startAgain();
    			// thread th_startAgain (startAgain);            	
            }

            // if (regex_match(message.begin(), message.end(), regex("^[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5},[-]{0,1}[0-9]{1,5}E$"))) {
            //     ///Data is location X, Y, Z Encoder
            //     vector<string> dataVec1;
            //     vector<int> dataVec2;
            //     split(message, 'E', dataVec1);
            //     split(dataVec1[0], ',', dataVec2);
            //     if (PosXYZ != dataVec2) {
            //         PosXYZ = dataVec2;
            //         sendPosXYZ(); } }

            // else
            //     if (socketDict.count("BaseStation"))
            //         ResponeSendCallback(socketDict["BaseStation"], message);
        }
        // usleep(50000);
    }
}

void listenClient(int listening)
{
    try {
        for (m.lock(); true; m.unlock())
        // while (true)
        {
            /// Wait for a connection
            socklen_t clientSize = sizeof(client);
            int clientSocket = accept(listening, (sockaddr*)&client, &clientSize);

            char host[NI_MAXHOST];      // Client's remote name
            // char service[NI_MAXSERV];   // Service (i.e. port) the client is connect on

            memset(host, 0, NI_MAXHOST); // same as memset(host, 0, NI_MAXHOST);
            // memset(service, 0, NI_MAXSERV);

            string IPAdd = inet_ntoa(client.sin_addr);
            // // printf("IP address is: %s\n", inet_ntoa(client.sin_addr));
            // // printf("port is: %d\n", (int) ntohs(client.sin_port));

            // if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
            //     cout << "C" << clientSocket << " >> " << host << " connected on port " << service << endl; }
            // else {
            //     inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            //     cout << "C" << clientSocket << " >> "  << host << " connected on port " << ntohs(client.sin_port) << endl; }
            
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            cout << "C" << clientSocket << " >> "  << host << " connected on port " << ntohs(client.sin_port) << endl;


            // socketDict[IPAdd+":"+to_string(clientSocket)] = clientSocket;
            socketDict["BaseStation"] = clientSocket;
            for (auto &i : socketDict)
                if (i.first != "BaseStation") {
                    // close(i.second);
                    socketDict[i.first] = 0; }

            /// Close listening socket
            // close(listening);

            /// Start received message
            // th_Receiveds[IPAdd+":"+to_string(clientSocket)] = thread(receivedCallBack, clientSocket);
            th_Receiveds["BaseStation"] = thread(receivedCallBack, clientSocket);
            // sendCallBack(clientSocket, useAs);
            // sendPosXYZ();
            checkConnection(); 
        } }
    catch (exception e) {
        cout << "# Listening error \n~swap\n"
             << e.what() << endl; }
}

int setupServer(int port)
{
    try {
        cout << "Server Starting..." << endl;
        /// Create a socket
        listening = socket(AF_INET, SOCK_STREAM, 0);
        if (listening == -1) {
            cerr << "Can't create a socket! Quitting" << endl;
            return -1; }

        /// Bind the ip address and port to a socket
        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);
        bind(listening, (sockaddr*)&hint, sizeof(hint));

        /// Tell Winsock the socket is for listening
        listen(listening, SOMAXCONN);
        cout  << "Listening to TCP clients at " << getMyIP() << " : " << port << endl;
        listenClient(listening); }
    catch (exception e) {
        cout << "# Setup Server error \n~\n" << e.what() << endl; }
}

bool changeTranspose()
{
    if (transpose == false) {
        transpose = true;
        cout << "Transpose mode ON" << endl; }
    else {
        transpose = false;
        cout << "Transpose mode OFF" << endl; }
    swap(PosXYZ[0], PosXYZ[1]);
    if (socketDict.count("BaseStation"))
        sendPosXYZ();
    return transpose;
}

// void keyEvent(string key)    ///For Arduino is NOT available
// {
//     vector<int> _temp = PosXYZ;
//     int x = 0, y = 1;
//     if (transpose == true) {
//         x = 1; y = 0; }

//     if (key == "[C")            //Right
//         PosXYZ[x] += 1;
//     else if (key == "[D")       //Left
//         PosXYZ[x] -= 1;
//     else if (key == "[A")       //Up
//         PosXYZ[y] -= 1;
//     else if (key == "[B")       //Down
//         PosXYZ[y] += 1;
//     else if (key == "[5")       //PageUp
//         PosXYZ[2] += 1;
//     else if (key == "[6")       //PageDown
//         PosXYZ[2] -= 1;
//    else if (key == ".")        //Dot (.)
//        setSerialport();
//    else if (key == ":")        //Double Dot (:)
//        changeTranspose();

//     for (int i = 0; i < 3; i++)
//         if ((PosXYZ[i] != _temp[i]) && socketDict.count("BaseStation") /*&& (_socketDict.ContainsKey("BaseStation"))*/){
//             sendPosXYZ();
//             break; }
//     // cout << "# X:" << PosXYZ[0] << " Y:" << PosXYZ[1] << " ∠:" << PosXYZ[2] << "°" << endl;
// }

void keyEvent(string key)   ///For Arduino is available
{
    string x = "x", y = "y", z = "z";
    if (transpose == true)
        swap(x, y);

    if (key == "[C")            //Right
        toArduino(x+"+");
    else if (key == "[D")       //Left
        toArduino(x+"-");
    else if (key == "[A")       //Up
        toArduino(y+"-");
    else if (key == "[B")       //Down
        toArduino(y+"+");
    else if (key == "[5")       //PageUp
        toArduino(z+"+");
    else if (key == "[6")       //PageDown
        toArduino(z+"-");
    else if (key == ".")        //Dot (.)
        setSerialport();
    else if (key == ":")        //Double Dot (:)
        changeTranspose();
    // cout << "# X:" << PosXYZ[0] << " Y:" << PosXYZ[1] << " ∠:" << PosXYZ[2] << "°" << endl;
}

void keyPress()
{
    string s = "";
    int i = 0;
    printf("# Set Location Mode \n");
    for (m.lock(); 1; m.unlock()){
        if (kbhit())
            s += getchar();
        if ((s.rfind("^") != (s.size() - 1)) && (s.rfind("[") != (s.size() - 1))) {
            if (s == "") {
                if (i == 1)
                    break;
                i++; }
            else {
                keyEvent(s);
                i = 0; }
            s = ""; } }
    printf("# Set Command Mode \n");
}

void setCommand()
{
    try {
        string Command;
        for (m.lock(); (true) && (toLowers(Command) != "quit"); m.unlock()) {
            getline(cin, Command);
            if (!isBlank(Command))
                Command = trim(Command);

            if (Command == "/") {           //Set Location Mode
                thread th_keyPress(keyPress);
                th_keyPress.join();
            }
            else if (Command == ".") {      //Set Serial Port
                setSerialport();
            }
            else if (Command == ",") {      //Check Connection
                cout << to_string(socketDict.size()) << endl;
                checkConnection();
            }
            else if (Command == ":") {      //Change Transpose
                changeTranspose();
            }
            else if (!isBlank(Command))
                ResponeSendCallback(socketDict["BaseStation"], Command); }
        sendCallBack(socketDict["BaseStation"],"quit");
        close(listening);
        close(socketDict["BaseStation"]);
        cout << "# Close App" << endl;
    }
    catch (exception e) {
        cout << "% setCommand error \n~\n" << e.what() << endl; }
}

void autoReconnect()
{
	for (int ar=0; ; ar++)
	{
		system("ar > backup_connect.txt");
		usleep(100000);
	}
}

int main()
{	
    // for (m.lock(); isBlank(useAs); m.unlock()){
    //     system("clear");
    //     printf("~ Welcome to Robot Core ~ \n");
    //     printf("Use as Robot: "); cin >> useAs;
    //     useAs = trim(useAs);
    //     if ((useAs == "1") ^ (toLowers(useAs) == "r1") ^ (toLowers(useAs) == "robot1")) useAs = "Robot1";
    //     else if ((useAs == "2") ^ (toLowers(useAs) == "r2") ^ (toLowers(useAs) == "robot2")) useAs = "Robot2";
    //     else if (((useAs == "3") ^ toLowers(useAs) == "r3") ^ (toLowers(useAs) == "robot3")) useAs = "Robot3";
    //     else useAs.clear(); }
    for (m.lock(); isBlank(serialPortCustom); m.unlock()){
    	system("clear");
    	setSerialport(); }
    getMyIP();
    thread th_setupServer(setupServer, 8686);
    thread th_fromArduino (fromArduino);
    thread th_setCommand (setCommand);
    th_setCommand.join();
    return 0;
}
