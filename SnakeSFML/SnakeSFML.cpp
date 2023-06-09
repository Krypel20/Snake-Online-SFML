﻿#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <SFML/Graphics.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <list>
#include <random>
#include <chrono>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")
const char DEFAULT_PORT[] = "20202";
int serverPort = 20202;
const char serverIp[] = "83.27.194.234";

class SnakeGame;
const int windowWidth = 1000;
const int windowHeight = 840;
const float outlineThickness = 2;
const int blockSize = 20;
const int frameLimit = 55;
const sf::Color backgroundColor(10, 10, 10);
const sf::Color foodColor = sf::Color::Red;
sf::Clock gameClock;
SOCKET tcpSocket = INVALID_SOCKET;
bool isGameOver;
bool gameStarted;
bool firstLaunch = true;

void connectToServer()
{
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    WSADATA wsaData;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return;
    }

    iResult = getaddrinfo(serverIp, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        tcpSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (tcpSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return;
        }
    }

    std::cout << "Laczenie z serwerem: " << serverIp << ":" << serverPort << std::endl;
    while (true)
    {
        if (connect(tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR) {
            std::cout << "Polaczenie udane...\n";
            break;
        }
        else
        {
            std::cout << "Nie udalo polaczyc sie z serwerem...\n";
       }
    }
}

bool checkConnection()
{
    char receivedData[512];

    fd_set set;
    FD_ZERO(&set);
    FD_SET(tcpSocket, &set);

    std::string message = "test!\n"; // Pierwsza wiadomość potwierdzająca połączenie klienta z serwerem
    while (true)
    {
        if (send(tcpSocket, message.c_str(), message.size() + 1, 0) == SOCKET_ERROR)
        {
            std::cout << "blad polaczenia...\n" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        else break;
    }

    if (recv(tcpSocket, receivedData, sizeof(receivedData), 0) == SOCKET_ERROR) {
        std::cout << "Nie udalo sie odebrac danych od serwera.\n" << std::endl;
        closesocket(tcpSocket);
        return false;
    }
    else { std::cout << "odebrano wiadomosc testowa!\n"; return true; }
}

struct SnakeObject
{
    friend class SnakeGame;
    sf::RectangleShape snakeShape;
    enum Direction { Up, Down, Left, Right } currentDirection;
    Direction nextDirection;

    struct SnakeSegment
    {
        int x, y;
        sf::Color segmentColor;

        SnakeSegment() : segmentColor(sf::Color::Blue) {}
        SnakeSegment(sf::Color color) : segmentColor(color) {}
    };

    SnakeSegment head;
    std::list<SnakeSegment> body;

    SnakeObject(){
        head = SnakeSegment(sf::Color(0,200,5));
        snakeShape = sf::RectangleShape(sf::Vector2f(blockSize, blockSize));
        snakeShape.setOutlineThickness(outlineThickness);
        snakeShape.setOutlineColor(sf::Color::Green);
        currentDirection = Direction::Up;
        nextDirection = Direction::Up;
    }

    void render()
    {
        body.clear();
        head.x = windowWidth / 2;
        head.y = windowHeight / 2;
        body.push_front(head);
    }

    void changeDirection()
    {
        // Aktualizacja kierunku na podstawie następnego kierunku
        currentDirection = nextDirection;

        // Aktualizacja pozycji głowy węża na podstawie kierunku
        head = body.front();
        switch (currentDirection) {
        case Direction::Up:
            head.y -= blockSize - blockSize / 2;
            break;
        case Direction::Down:
            head.y += blockSize - blockSize / 2;
            break;
        case Direction::Left:
            head.x -= blockSize - blockSize / 2;
            break;
        case Direction::Right:
            head.x += blockSize - blockSize / 2;
            break;
        }
    }

    void addSegment()
    {
        SnakeObject::SnakeSegment tail = body.back(); // Pobranie aktualnego ogona węża
        SnakeObject::SnakeSegment newTail;
        newTail.x = tail.x;
        newTail.y = tail.y;
        // dodanie nowego bloku do węża
        body.push_back(tail);
    }

    bool checkEat(sf::CircleShape food)
    {
        if (head.x == food.getPosition().x && head.y == food.getPosition().y ||
            (head.x == food.getPosition().x - blockSize/2 && head.y == food.getPosition().y - blockSize / 2) ||
            (head.x == food.getPosition().x + blockSize / 2 && head.y == food.getPosition().y + blockSize / 2))
        {
            return true;
        }
        return false;
    }

    bool checkCollision()
    {
        // Sprawdzenie kolizji głowy węża z ciałem lub ścianami
        int headX = head.x;
        int headY = head.y;

        // Kolizja ze ścianami
        if (head.x <= 0 || head.x >= windowWidth-blockSize || head.y <= 0 || head.y >= windowHeight - blockSize) 
        {
            return true;
        }

        // Kolizja z ciałem węża
        for (auto it = std::next(body.begin()); it != body.end(); ++it) 
        {
            if (head.x == it->x && head.y == it->y) {
                return true;
            }
        }
        return false;
    }
    
    void render(sf::RenderWindow &window)
    {
        for (const auto& segment : body)
        {
            snakeShape.setPosition(segment.x, segment.y);
            snakeShape.setFillColor(segment.segmentColor);
            window.draw(snakeShape);
        }
    }

    void handleInput()
    {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && currentDirection != Direction::Down) {
                nextDirection = Direction::Up;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && currentDirection != Direction::Up) {
                nextDirection = Direction::Down;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && currentDirection != Direction::Right) {
                nextDirection = Direction::Left;
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && currentDirection != Direction::Left) {
                nextDirection = Direction::Right;
            }
    }
};

class SnakeGame {
    friend struct SnakeObject;
public:
    SnakeGame();
    void runGame();
    void gameOver();
    void menu();
    bool isGamePaused;
    bool enemyGameOver;
    bool playerReady;
    bool bothReady();
    bool isGameRunning;
    bool enemyReady;
    sf::CircleShape food;

private:
    //void handleEvents(sf::RenderWindow& window, sf::Event& event, sf::Text& inputText);
    //void drawMenu(sf::RenderWindow& window, sf::Text& inputText);
    void updateScore();
    void checkEvents();
    void gameUpdate();
    void gameInfo();
    void render();
    void generateFood();
    void renderObjects();
    void checkWindowEvents(sf::RenderWindow& window);
    void receiveData();
    void sendScore();
    void sendGameOver();
    void sendDecision();
    void printGameStart();
    void printGameRestart();
    void printGameOver();
    void printGameWaitingForConnection();
    void setGame();

    int P1Score; //zmienna do przechowywania aktualnego wyniku
    int P2Score;

    SnakeObject playerSnake;
    sf::RenderWindow window;
    sf::Font font; // czcionka do wyświetlania tekstu
    sf::Text scoreText; // tekst wyświetlający aktualny wynik
    sf::Text gameOverText; // tekst wyświetlający koniec gry i punkty graczy
    sf::Text gameInfoText;
    sf::Text gameStartText;
};

SnakeGame::SnakeGame() : playerSnake(), window(sf::VideoMode(windowWidth, windowHeight), "Snake Online - Piotr Krypel", sf::Style::Default)
{
        window.clear(backgroundColor);
        window.setFramerateLimit(frameLimit);
        setGame();
        playerReady = false;
        enemyReady = false;

        if (!font.loadFromFile("yoster.ttf")) { // załaduj czcionkę z pliku yoster.ttf
            std::cout << "Blad: nie udalo się zaladowac czcionki!" << std::endl;
        }
}

void SnakeGame::renderObjects()
{
    playerSnake.render();
    generateFood();
}

void SnakeGame::setGame()
{
    isGameRunning = false;
    isGamePaused = false;
    enemyGameOver = false;
    isGameOver = false;
    P1Score = 0;
    P2Score = 0;
}

void SnakeGame::checkWindowEvents(sf::RenderWindow& window)
{
    sf::Event event;
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed) 
        {
            closesocket(tcpSocket);
            window.close();
        }
    }
}

void SnakeGame::updateScore()
{
    // Aktualizacja tekstu wyświetlającego aktualny wynik
    scoreText.setFont(font);
    scoreText.setCharacterSize(22);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(windowWidth/2 - 130, 30);
    scoreText.setString("Ty: " + std::to_string(P1Score) + " Przeciwnik: " + std::to_string(P2Score));
}

void SnakeGame::gameInfo()
{
    gameInfoText.setFont(font);
    gameInfoText.setCharacterSize(22);
    gameInfoText.setFillColor(sf::Color::White);
    gameInfoText.setPosition(windowWidth/2-220, 5);
    if (!isGameRunning)
    {
        if (enemyReady && playerReady)
        {
            gameInfoText.setString("Ty: gotowy     Przeciwnik: gotowy");
        }
        if (!enemyReady && !playerReady)
        {
            gameInfoText.setString("Ty: niegotowy  Przeciwnik: niegotowy");
        }
        if (!enemyReady && playerReady)
        {
            gameInfoText.setString("Ty: gotowy     Przeciwnik: niegotowy");
        }
        if (enemyReady && !playerReady)
        {
            gameInfoText.setString("Ty: niegotowy  Przeciwnik: gotowy");
        }
    }
    else
    {
        if (enemyGameOver && isGameOver)
        {
            gameInfoText.setString("Ty: skuty      Przeciwnik: skuty");
        }
        if(!enemyGameOver && !isGameOver)
        {
            gameInfoText.setString("Ty: w grze     Przeciwnik: w grze");
        }
        if (!enemyGameOver && isGameOver)
        {
            gameInfoText.setString("Ty: skuty      Przeciwnik: w grze");
        }
        if (enemyGameOver && !isGameOver)
        {
            gameInfoText.setString("Ty: w grze     Przeciwnik: skuty");
        }
    }
}

void SnakeGame::printGameStart()
{
    gameStartText.setFont(font);
    gameStartText.setCharacterSize(40);
    gameStartText.setFillColor(sf::Color::White);
    gameStartText.setPosition(windowWidth / 2 - 350, windowHeight / 2);
    gameStartText.setString("Oczekiwanie na gotowosc graczy\n\t\tWcisnij dowolny klawisz");
}  

void SnakeGame::printGameRestart()
{
    gameStartText.setFont(font);
    gameStartText.setCharacterSize(40);
    gameStartText.setFillColor(sf::Color::White);
    gameStartText.setPosition(windowWidth / 2 - 350, windowHeight / 2);
    gameStartText.setString("\t\t Wcisnij dowolny klawisz\n jesli chcesz zagrac jeszcze raz");
}

void SnakeGame::printGameWaitingForConnection()
{
    gameStartText.setFont(font);
    gameStartText.setCharacterSize(39);
    gameStartText.setFillColor(sf::Color::White);
    gameStartText.setPosition(windowWidth / 2 - 550, windowHeight / 2 - 50);
    gameStartText.setString("     Oczekiwanie na polaczenie z przeciwnikiem...");
}

void SnakeGame::printGameOver()
{
    gameOverText.setFont(font);
    gameOverText.setOutlineThickness(1);
    gameOverText.setOutlineColor(sf::Color::Yellow);
    gameOverText.setCharacterSize(40);
    gameOverText.setFillColor(sf::Color::Black);
    gameOverText.setPosition(windowWidth / 2 - 150, windowHeight / 2 - 200);
    if (P1Score > P2Score) {
        gameOverText.setString("   WYGRALES!\n\tGracz: " + std::to_string(P1Score) + "\nPrzeciwnik : " + std::to_string(P2Score));
    }
    else if (P1Score == P2Score) {
        gameOverText.setString("      REMIS!\n\tGracz: " + std::to_string(P1Score) + "\nPrzeciwnik: " + std::to_string(P2Score));
    }
    else {
        gameOverText.setString(" PRZEGRALES!\n\tGracz " + std::to_string(P1Score) + "\nPrzeciwnik: " + std::to_string(P2Score));
    }
}

void SnakeGame::checkEvents()
{
    if (!playerReady)
    {
        sf::Event ispressed;
        while (window.pollEvent(ispressed))
        {
            if (ispressed.type == sf::Event::KeyPressed)
            {
                std::cout << "Klawisz zostal wcisniety\n";
                sendDecision();
            }
        }
    }
}

void SnakeGame::menu()
{
    checkWindowEvents(window);
    window.clear(backgroundColor);
    printGameWaitingForConnection();
    sf::Event event; 
    connectToServer();
    while (window.isOpen() && !playerReady && !enemyReady)
    {
        window.draw(gameStartText);
        checkWindowEvents(window);
        window.display();
        if (checkConnection()) { firstLaunch = false; break; }
    }
}

void SnakeGame::runGame()
{
    while(window.isOpen())
    {
        int remainingTime = 3+1;
        if (firstLaunch) {menu();}
        setGame();
        checkWindowEvents(window);

        while (window.isOpen() && !bothReady())
        {
            checkWindowEvents(window);
            printGameStart();
            receiveData();
            render();
            checkEvents();
        }

        while (window.isOpen() && bothReady() && !isGameRunning)
        {
            renderObjects();
            checkWindowEvents(window);
            gameStartText.setCharacterSize(100);
            gameStartText.setPosition(windowWidth / 2 - blockSize, windowHeight / 2);
            gameStartText.setFillColor(sf::Color::Red);
            render();
            receiveData();

            if (gameClock.getElapsedTime() >= sf::seconds(1))
            {
                remainingTime -= 1;
                std::cout << "Czas do startu: " << remainingTime << std::endl;
                gameClock.restart();

                if (remainingTime > 0)
                {
                    gameStartText.setString(std::to_string(remainingTime));
                }
            }
            if (remainingTime <= 0)
            {
                isGameRunning = true;
                std::cout << "Rozpoczeto gre!\n";
            }
        }

        while (window.isOpen() && isGameRunning)
        {
            checkWindowEvents(window);
            if (isGameOver && !enemyGameOver)
            {
                render();
                receiveData();
                updateScore();
                printGameOver();
            }
            else if (isGameOver && enemyGameOver)
            {
                render();
                printGameOver();
                enemyReady = false;
                playerReady = false;
                isGameRunning = false;

                while (!bothReady())
                {
                    render();
                    printGameOver();
                    printGameRestart();
                    checkEvents();
                    receiveData();
                }
                break;
            }
            else //działanie gry
            {
                render();
                playerSnake.handleInput();
                gameUpdate();
                receiveData();
                updateScore();
                if (P2Score < P1Score && enemyGameOver) { gameOver(); }
            }
        }
    }
}

void SnakeGame::gameOver()
{
    isGameOver = true;
    isGamePaused = true; //zatrzymanie gry
    printGameOver();
    sendGameOver();
}

void SnakeGame::gameUpdate()
{
    if (!isGamePaused && !isGameOver)
    {
        playerSnake.changeDirection();

        // ruch wężą przed siebie
        playerSnake.body.push_front(playerSnake.head);

        // Sprawdzenie kolizji z jedzeniem oraz z samym sobą
        if (playerSnake.checkEat(food))
        {
            SnakeGame::generateFood();
            SnakeGame::P1Score++; //dodanie punktów
            SnakeGame::sendScore();//wysłanie aktualnego wyniku na serwer
            for (int i = 0; i < 2; ++i) //zwiększenie o dwa bloki
            {
                playerSnake.addSegment();
            }
        }
        else 
        {
            playerSnake.body.pop_back();
        }

        if (playerSnake.checkCollision()) 
        {
            gameOver();
        }
    }
}

void SnakeGame::render()
{
        window.clear(backgroundColor);
        window.draw(gameInfoText);
        gameInfo();

        // renderowanie węża gracza
        playerSnake.render(window);

        // Renderowanie jedzenia
        if (isGameRunning)
        {
            window.draw(scoreText);
            window.draw(food);
        }
        if (isGameOver) { window.draw(gameOverText); }
        if (!isGameRunning) { window.draw(gameStartText); }
        window.display();
}

void SnakeGame::generateFood()
{
    // Generowanie nowego jedzenia na losowej pozycji
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, windowWidth / blockSize - 1);
    std::uniform_int_distribution<> disY(0, windowHeight / blockSize - 1);
    int foodX = disX(gen) * blockSize;
    int foodY = disY(gen) * blockSize;

    /*food.setSize(sf::Vector2f(blockSize, blockSize));*/
    food.setOutlineThickness(outlineThickness);
    food.setRadius(float(blockSize/2-outlineThickness/2));
    food.setFillColor(foodColor);
    food.setPosition(foodX, foodY);
}

bool SnakeGame::bothReady()
{
    if (playerReady && enemyReady) return true;
    else return false;
}

int main(int argc, char** argv)
{
    std::cout << "---------Uruchomienie Gry Snake----------\n";
    SnakeGame game;
    game.runGame();
    closesocket(tcpSocket);
    WSACleanup();
    return 0;
}

void SnakeGame::receiveData()
{
    int receivedSize;
    char receivedData[512];
    int score;
    std::string message;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(tcpSocket, &set);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Sprawdzenie, czy istnieją dane gotowe do odczytu
    int result = select(0, &set, NULL, NULL, &timeout);
    if (result == 0) {
        return;
    }
    // Odebranie wiadomości
    if (recv(tcpSocket, (char*)&receivedData, sizeof(int), 0) > 0)
    {
        if (std::isdigit(receivedData[0]))
        {
            P2Score = std::stoi(receivedData);
            std::cout << "Wynik przeciwnika: " << P2Score << std::endl;
        }
        if (std::isalpha(receivedData[0]))
        {
            std::string receivedString(receivedData);
            receivedString.resize(strnlen_s(receivedData, 2));

            if (receivedString == "GO")
            {
                std::cout << "Przeciwnik przegral\n";
                enemyGameOver = true;
            }
            if (receivedString == "RD")
            {
                std::cout << "Przeciwnik gotowy\n";
                enemyReady = true;
            }
        }
    }
}

void SnakeGame::sendScore()
{
    std::string message = std::to_string(P1Score);
    if (send(tcpSocket, message.c_str(), message.length(), 0) > 0)
    {
        std::cout << "Twoj wynik: " << P1Score << std::endl;
    }
}

void SnakeGame::sendGameOver()
{
    if (isGameOver)
    {
        std::string message = "GO"; //GameOver
        if (send(tcpSocket, message.c_str(), message.length(), 0) != SOCKET_ERROR)
        {
            std::cout << "Przegrales\n" << std::endl;
        }
        else {
            std::cout << "problem z wysłaniem prosby o zakonczenie gry\n" << std::endl;
        }
    }
}

void SnakeGame::sendDecision()
{
    if (!playerReady)
    {
        std::string message = "RD"; //Ready
        if (send(tcpSocket, message.c_str(), message.length(), 0) != SOCKET_ERROR)
        {
            playerReady = true;
            std::cout << "jestes gotowy do gry\n" << std::endl;
        }
        else {
            std::cout << "problem z wysłaniem gotowosci\n" << std::endl;
        }
    }
}