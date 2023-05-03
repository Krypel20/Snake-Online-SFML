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
const char serverIp[] = "83.27.9.159";

//GAME SETS
const int windowWidth = 980;
const int windowHeight = 660;
const int blockSize = 20;
const int frameLimit = 55;
const sf::Color backgroundColor = sf::Color::Black;
const sf::Color snakeColor = sf::Color::Green;
const sf::Color foodColor = sf::Color::Red;
float deltaT;
sf::Clock gameClock;
SOCKET tcpSocket = INVALID_SOCKET;
sf::RectangleShape snakeShape(sf::Vector2f(blockSize, blockSize)); //wąż
bool isGameOver;
bool gameStarted;

struct SnakeSegment
{
    int x, y;
    sf::RectangleShape segmentRectShape;
    sf::Color segmentColor; // dodane pole koloru
};

class SnakeGame {
public:
    SnakeGame();
    void runGame();
    void gameOver();
    void printGameStart();
    void printGameRestart();
    void printGameOver();
    bool isGamePaused;
    bool enemyGameOver;
    bool enemyReady;
    bool playerReady;
    bool bothReady();
    bool gameStart;

private:
    void updateScore();
    void handleInput();
    void checkEvents();
    void gameUpdate();
    void gameInfo();
    void render();
    void generateFood();
    void renderObjects();
    bool checkCollision();

    void receiveData();
    void sendScore();
    void sendGameOver();
    void sendDecision();

    void setGame();

    int P1Score; //zmienna do przechowywania aktualnego wyniku
    int P2Score;
    sf::RenderWindow window;
    std::list<SnakeSegment> snake;
    SnakeSegment head;
    sf::RectangleShape food;
    enum Direction { Up, Down, Left, Right } currentDirection;
    Direction nextDirection;
    sf::Font font; // czcionka do wyświetlania tekstu
    sf::Text scoreText; // tekst wyświetlający aktualny wynik
    sf::Text gameOverText; // tekst wyświetlający koniec gry i punkty graczy
    sf::Text gameInfoText;
    sf::Text gameStartText;
};

SnakeGame::SnakeGame() : window(sf::VideoMode(windowWidth, windowHeight), "Snake Game Online - Piotr Krypel")
{
        window.clear(backgroundColor);
        window.setFramerateLimit(frameLimit);
        renderObjects();
        setGame();
        playerReady = false;
        enemyReady = false;

        if (!font.loadFromFile("yoster.ttf")) { // załaduj czcionkę z pliku yoster.ttf
            std::cout << "Blad: nie udalo się zaladowac czcionki!" << std::endl;
        }
}

void SnakeGame::renderObjects()
{
    snake.clear();
    head.x = windowWidth / 2;
    head.y = windowHeight / 2;
    snake.push_front(head);

    currentDirection = Direction::Up;
    nextDirection = Direction::Up;
    generateFood();
}

void SnakeGame::setGame()
{
    gameStart = false;
    isGamePaused = false;
    enemyGameOver = false;
    isGameOver = false;
    P1Score = 0;
    P2Score = 0;
}

void SnakeGame::updateScore()
{
    // Aktualizacja tekstu wyświetlającego aktualny wynik
    scoreText.setFont(font);
    scoreText.setCharacterSize(22);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 30);
    scoreText.setString("\t\t\t\t\t\t\t\t\t\t\t\tGracz: " + std::to_string(P1Score) + " Przeciwnik: " + std::to_string(P2Score));
}

void SnakeGame::gameInfo()
{
    gameInfoText.setFont(font);
    gameInfoText.setCharacterSize(22);
    gameInfoText.setFillColor(sf::Color::White);
    gameInfoText.setPosition(10, 5);
    if (!gameStart)
    {
        if (enemyReady && playerReady)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Gracz: gotowy # Przeciwnik: gotowy");
        }
        if (!enemyReady && !playerReady)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Gracz: niegotowy # Przeciwnik: niegotowy");
        }
        if (!enemyReady && playerReady)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Gracz: gotowy # Przeciwnik: niegotowy");
        }
        if (enemyReady && !playerReady)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Gracz: niegotowy # Przeciwnik: gotowy");
        }
    }
    else
    {
        if (enemyGameOver && isGameOver)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Gracz: skuty # Przeciwnik: skuty");
        }
        if(!enemyGameOver && !isGameOver)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Gracz: w grze # Przeciwnik: w grze");
        }
        if (!enemyGameOver && isGameOver)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Gracz: skuty # Przeciwnik: w grze");
        }
        if (enemyGameOver && !isGameOver)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Gracz: w grze # Przeciwnik: skuty");
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
    gameStartText.setString("\t\tWcisnij dowolny klawisz\n jesli chcesz zagrac jeszcze raz");
}

void SnakeGame::printGameOver()
{
    gameOverText.setFont(font);
    gameOverText.setOutlineThickness(2);
    gameOverText.setOutlineColor(sf::Color::Yellow);
    gameOverText.setCharacterSize(40);
    gameOverText.setFillColor(sf::Color::Black);
    gameOverText.setPosition((windowWidth - windowHeight) / 2 + 200, windowHeight / 2 - 150);
    if (P1Score > P2Score) {
        gameOverText.setString("WYGRALES!\nGracz: " + std::to_string(P1Score) + "\nPrzeciwnik: " + std::to_string(P2Score));
    }
    else if (P1Score == P2Score) {
        gameOverText.setString("REMIS!\nGracz: " + std::to_string(P1Score) + "\nPrzeciwnik: " + std::to_string(P2Score));
    }
    else {
        gameOverText.setString("PRZEGRALES!\nGracz: " + std::to_string(P1Score) + "\nPrzeciwnik: " + std::to_string(P2Score));
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
                std::cout << "KLAWISZ ZOSTAL WCISNIETY\n";
                sendDecision();
            }
        }
    }

    sf::Event event;
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
    }
}

void SnakeGame::runGame()
{
    while(window.isOpen())
    {
        setGame();
        renderObjects();
        int remainingTime = 3 + 1; //timer 3 sekundy

        while (window.isOpen() && !bothReady())
        {
            printGameStart();
            receiveData();
            render();
            checkEvents();
        }

        while (window.isOpen() && bothReady() && !gameStart)
        {
            gameStartText.setCharacterSize(100);
            gameStartText.setPosition(windowWidth / 2, windowHeight / 2);
            gameStartText.setFillColor(sf::Color::Red);

            render();
            receiveData();

            if (gameClock.getElapsedTime() >= sf::seconds(1))
            {
                remainingTime -= 1;
                std::cout << "Pozostaly czas do startu: " << remainingTime << std::endl;
                gameClock.restart();

                if (remainingTime > 0)
                {
                    gameStartText.setString(std::to_string(remainingTime));
                }
            }
            if (remainingTime <= 0)
            {
                gameStart = true;
                std::cout << "Rozpoczeto gre!\n";
            }
        }

        while (window.isOpen() && gameStart)
        {
            deltaT = gameClock.restart().asSeconds();
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }
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
                gameStart = false;
                sf::sleep(sf::seconds(2));
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
                handleInput();
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

void SnakeGame::handleInput()
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

void SnakeGame::gameUpdate()
{
    if (!isGamePaused && !isGameOver)
    {
        // Aktualizacja kierunku na podstawie następnego kierunku
        currentDirection = nextDirection;

        // Aktualizacja pozycji głowy węża na podstawie kierunku
        head = snake.front();
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

        // ruch wężą przed siebie
        snake.push_front(head);
        // Sprawdzenie kolizji z jedzeniem oraz z samym sobą
        if (head.x == food.getPosition().x && head.y == food.getPosition().y ||
            (head.x - blockSize / 2 == food.getPosition().x && head.y + blockSize / 2 == food.getPosition().y) ||
            (head.x + blockSize / 2 == food.getPosition().x && head.y - blockSize / 2 == food.getPosition().y))
        {
            generateFood();
            P1Score++; //dodanie punktów
            sendScore();//wysłanie aktualnego wyniku na serwer
            for (int i = 0; i < 2; ++i)
            {
                SnakeSegment tail = snake.back(); // Pobranie aktualnego ogona węża
                SnakeSegment newTail;
                newTail.x = tail.x;
                newTail.y = tail.y;
                // dodanie nowego bloku do węża
                snake.push_back(newTail);
            }
        }
        else 
        {
            snake.pop_back();
        }

        if (checkCollision()) 
        {
            // Zderzenie z samym sobą - koniec gry
            gameOver();
        }
    }
}

void SnakeGame::render()
{
    window.clear(backgroundColor);
    window.draw(gameInfoText);
    gameInfo();

    // Rysowanie węża
    for (const auto& segment : snake) 
    {
        snakeShape.setPosition(segment.x, segment.y);
        snakeShape.setFillColor(snakeColor);
        window.draw(snakeShape);
    }

    // Rysowanie jedzenia
    if (gameStart)
    {
        window.draw(food);
        window.draw(scoreText);
    }
    if (isGameOver) { window.draw(gameOverText); }
    if (!gameStart) { window.draw(gameStartText); }
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

    food.setSize(sf::Vector2f(blockSize, blockSize));
    food.setPosition(foodX, foodY);
    food.setFillColor(foodColor);
}

bool SnakeGame::checkCollision()
{
    // Sprawdzenie kolizji głowy węża z ciałem lub ścianami
    int headX = head.x;
    int headY = head.y;

    // Kolizja ze ścianami
    if (headX < 0 || headX >= windowWidth || headY < 0 || headY >= windowHeight) {
        return true;
    }

    // Kolizja z ciałem węża
    for (auto it = std::next(snake.begin()); it != snake.end(); ++it) {
        if (headX == it->x && headY == it->y) {
            return true;
        }
    }
    return false;
}

bool SnakeGame::bothReady()
{
    if (playerReady && enemyReady) return true;
    else return false;
}

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
        return ;
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
            std::this_thread::sleep_for(std::chrono::seconds(1)); // program czeka 1 s
        }
    }
}

void checkConnection()
{
    std::string message = "test!\n"; // Pierwsza wiadomość potwierdzająca połączenie klienta z serwerem
    while(true)
    {
        if (send(tcpSocket, message.c_str(), message.size() + 1, 0) == SOCKET_ERROR)
        {
            std::cout << "blad polaczenia...\n" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        else break;
    }
    std::cout << "sprawdzanie polaczenia z drugim klientem\n";
    // Sprawdzenie czy klient moze odebrac dane
    char recMsg[512];
    if (recv(tcpSocket, recMsg, sizeof(recMsg), 0) == SOCKET_ERROR) {
        std::cout << "Nie udalo sie odebrac danych od serwera.\n" << std::endl;
        closesocket(tcpSocket);
        return;
    }
    else { std::cout << "odebrano wiadomosc testowa!\n"; return; }
}


int main(int argc, char** argv)
{
    std::cout << "---------Rozpoczecie programu----------\n";
    connectToServer();
    checkConnection();
    SnakeGame game;
    game.runGame();
    std::cout << "---------Program zostal zamkniety----------\n";
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

    // Utworzenie zbioru deskryptorów gniazd sieciowych
    fd_set set;
    FD_ZERO(&set);
    FD_SET(tcpSocket, &set);

    // Ustawienie czasu oczekiwania na dane gotowe do odczytu
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Sprawdzenie, czy istnieją dane gotowe do odczytu
    int result = select(0, &set, NULL, NULL, &timeout);
    if (result == SOCKET_ERROR) {
        std::cout << "Błąd w funkcji select(): " << WSAGetLastError() << std::endl;
        return;
    }
    else if (result == 0) {
        // Brak danych gotowych do odczytu
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