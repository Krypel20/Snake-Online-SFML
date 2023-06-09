#define _WINSOCK_DEPRECATED_NO_WARNINGS

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
const char serverIp[] = "83.27.200.217";

class SnakeGame;
//GAME SETS
const int windowWidth = 1000;
const int windowHeight = 840;
const float outlineThickness = 2;
const int blockSize = 20;
const int frameLimit = 55;
const sf::Color backgroundColor(10, 10, 10, 255);
const sf::Color foodColor = sf::Color::Red;
float deltaT;
sf::Clock gameClock;
SOCKET tcpSocket = INVALID_SOCKET;
bool isGameOver;
bool gameStarted;

struct SnakeObject
{
    bool isPlayer;
    struct SnakeSegment
    {
        int x, y;
        sf::Color segmentColor;

        SnakeSegment(bool isHead = false) :segmentColor(isHead ? sf::Color(30, 205, 0) : sf::Color(253, 230, 0)) {}
        SnakeSegment(sf::Color color) : segmentColor(color) {}
    };

    friend class SnakeGame;
    SnakeSegment head;
    std::list<SnakeSegment> body;
    sf::Color bodyColor;
    sf::Color headColor;
    sf::RectangleShape snakeShape;
    enum Direction { Up, Down, Left, Right } currentDirection;
    Direction nextDirection;

    SnakeObject() : bodyColor(30, 165, 0), headColor(253, 230, 0) {
        head = SnakeSegment(true);
        snakeShape = sf::RectangleShape(sf::Vector2f(blockSize, blockSize));
        snakeShape.setOutlineThickness(outlineThickness);
        snakeShape.setOutlineColor(sf::Color::Green);
        currentDirection = Direction::Up;
        nextDirection = Direction::Up;
        isPlayer = true;
    }

    SnakeObject(sf::Color color) : bodyColor(sf::Color::Magenta), headColor(color) {
        head = SnakeSegment(color);
        snakeShape = sf::RectangleShape(sf::Vector2f(blockSize, blockSize));
        snakeShape.setOutlineThickness(outlineThickness);
        snakeShape.setOutlineColor(sf::Color::Cyan);
        currentDirection = Direction::Up;
        nextDirection = Direction::Up;
        isPlayer = false;
    }

    void render()
    {
        if (isPlayer)
        {
            body.clear();
            head.x = windowWidth / 2 - 200;
            head.y = windowHeight / 2;
            body.push_front(head);
        }
        else
        {
            body.clear();
            head.x = windowWidth / 2 + 200;
            head.y = windowHeight / 2;
            body.push_front(head);
        }
    }

    void changeDirection()
    {
        // Aktualizacja kierunku na podstawie nast�pnego kierunku
        currentDirection = nextDirection;

        // Aktualizacja pozycji g�owy w�a na podstawie kierunku
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
        SnakeObject::SnakeSegment tail = body.back(); // Pobranie aktualnego ogona w�a
        SnakeObject::SnakeSegment newTail(bodyColor);
        newTail.x = tail.x;
        newTail.y = tail.y;
        // dodanie nowego bloku do w�a
        body.push_back(newTail);
    }

    bool checkEat(sf::CircleShape food)
    {
        if (head.x == food.getPosition().x && head.y == food.getPosition().y ||
            (head.x - blockSize / 2 == food.getPosition().x && head.y + blockSize / 2 == food.getPosition().y) ||
            (head.x + blockSize / 2 == food.getPosition().x && head.y - blockSize / 2 == food.getPosition().y))
        {
            return true;
        }
        return false;
    }

    bool checkCollision()
    {
        // Sprawdzenie kolizji g�owy w�a z cia�em lub �cianami
        int headX = head.x;
        int headY = head.y;

        // Kolizja ze �cianami
        if (head.x < 0 || head.x >= windowWidth || head.y < 0 || head.y >= windowHeight) {
            return true;
        }

        // Kolizja z cia�em w�a
        for (auto it = std::next(body.begin()); it != body.end(); ++it) {
            if (head.x == it->x && head.y == it->y) {
                return true;
            }
        }
        return false;
    }

    void render(sf::RenderWindow& window)
    {
        for (const auto& segment : body)
        {
            snakeShape.setPosition(segment.x, segment.y);
            snakeShape.setFillColor(segment.segmentColor);
            window.draw(snakeShape);
        }
    }

    void sendDirection(char dir)
    {
        std::string message;
        if (dir == 'W') message = "W";
        if (dir == 'S') message = "S";
        if (dir == 'D') message = "D";
        if (dir == 'A') message = "A";

        if (send(tcpSocket, message.c_str(), message.length(), 0) != SOCKET_ERROR)
        {
            std::cout << "Sended direction - " << message << std::endl;
        }
    }

    void handleInput(bool isPlayer)
    {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && currentDirection != Direction::Down) {
            nextDirection = Direction::Up;
            sendDirection('W');
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && currentDirection != Direction::Up) {
            nextDirection = Direction::Down;
            sendDirection('S');
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && currentDirection != Direction::Right) {
            nextDirection = Direction::Left;
            sendDirection('A');
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && currentDirection != Direction::Left) {
            nextDirection = Direction::Right;
            sendDirection('D');
        }
    }
};

class SnakeGame {
    friend struct SnakeObject;
public:
    SnakeGame();
    void runGame();
    void gameOver();
    void printGameStart();
    void printGameRestart();
    void printGameOver();
    bool isGamePaused;
    bool enemyGameOver;
    bool playerReady;
    bool bothReady();
    bool gameStart;
    bool enemyReady;
    sf::CircleShape food;

private:
    void updateScore();
    void checkEvents();
    void gameUpdate();
    void gameInfo();
    void render();
    void generateFood();
    void renderObjects();

    void receiveData();
    void sendScore();
    void sendGameOver();
    void sendDecision();

    void setGame();

    int P1Score; //zmienna do przechowywania aktualnego wyniku
    int P2Score;

    SnakeObject playerSnake;
    SnakeObject enemySnake;
    sf::RenderWindow window;
    sf::Font font; // czcionka do wy�wietlania tekstu
    sf::Text scoreText; // tekst wy�wietlaj�cy aktualny wynik
    sf::Text gameOverText; // tekst wy�wietlaj�cy koniec gry i punkty graczy
    sf::Text gameInfoText;
    sf::Text gameStartText;
};

SnakeGame::SnakeGame() : playerSnake(), enemySnake(sf::Color::Blue), window(sf::VideoMode(windowWidth, windowHeight), "Snake Game Online - Piotr Krypel", sf::Style::Default)
{
    window.clear(backgroundColor);
    window.setFramerateLimit(frameLimit);
    renderObjects();
    setGame();
    playerReady = false;
    enemyReady = false;

    if (!font.loadFromFile("yoster.ttf")) { // za�aduj czcionk� z pliku yoster.ttf
        std::cout << "Blad: nie udalo si� zaladowac czcionki!" << std::endl;
    }
}

void SnakeGame::renderObjects()
{
    playerSnake.render();
    enemySnake.render();
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
    // Aktualizacja tekstu wy�wietlaj�cego aktualny wynik
    scoreText.setFont(font);
    scoreText.setCharacterSize(22);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 30);
    scoreText.setString("\t\t\t\t\t\t\t\t\t\t\t\tTy: " + std::to_string(P1Score) + " Przeciwnik: " + std::to_string(P2Score));
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
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Ty: gotowy # Przeciwnik: gotowy");
        }
        if (!enemyReady && !playerReady)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Ty: niegotowy # Przeciwnik: niegotowy");
        }
        if (!enemyReady && playerReady)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Ty: gotowy # Przeciwnik: niegotowy");
        }
        if (enemyReady && !playerReady)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Ty: niegotowy # Przeciwnik: gotowy");
        }
    }
    else
    {
        if (enemyGameOver && isGameOver)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Ty: skuty # Przeciwnik: skuty");
        }
        if (!enemyGameOver && !isGameOver)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Ty: w grze # Przeciwnik: w grze");
        }
        if (!enemyGameOver && isGameOver)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Ty: skuty # Przeciwnik: w grze");
        }
        if (enemyGameOver && !isGameOver)
        {
            gameInfoText.setString("dT: " + std::to_string(deltaT) + "\t\t\t\t Ty: w grze # Przeciwnik: skuty");
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
    while (window.isOpen())
    {
        sf::Event event;
        setGame();
        renderObjects();
        int remainingTime = 3 + 1; //timer 3 sekundy

        while (window.isOpen() && !bothReady())
        {
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) {
                    sendGameOver();
                    window.close();
                }
            }
            printGameStart();
            receiveData();
            render();
            checkEvents();
        }

        while (window.isOpen() && bothReady() && !gameStart)
        {
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) {
                    sendGameOver();
                    window.close();
                }
            }
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
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) {
                    sendGameOver();
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
            else //dzia�anie gry
            {
                render();
                playerSnake.handleInput(true);
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
        enemySnake.changeDirection();

        // ruch w꿹 przed siebie
        playerSnake.body.push_front(playerSnake.head);
        enemySnake.body.push_front(enemySnake.head);

        // Sprawdzenie kolizji z jedzeniem oraz z samym sob�
        if (playerSnake.checkEat(food))
        {
            SnakeGame::generateFood();
            SnakeGame::P1Score++; //dodanie punkt�w
            SnakeGame::sendScore();//wys�anie aktualnego wyniku na serwer
            for (int i = 0; i < 2; ++i) //zwi�kszenie o dwa bloki
            {
                playerSnake.addSegment();
            }
        }
        else
        {
            playerSnake.body.pop_back();
            enemySnake.body.pop_back();
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

    // Rysowanie w�a gracza
    playerSnake.render(window);

    //Rysowanie w�a przeciwnika
    enemySnake.render(window);

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

    /*food.setSize(sf::Vector2f(blockSize, blockSize));*/
    food.setOutlineThickness(outlineThickness);
    food.setRadius(float(blockSize / 2 - outlineThickness / 2));
    food.setFillColor(foodColor);
    food.setPosition(foodX, foodY);
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
            std::this_thread::sleep_for(std::chrono::seconds(1)); // program czeka 1 s
        }
    }
}

void checkConnection()
{
    std::string message = "test!\n"; // Pierwsza wiadomo�� potwierdzaj�ca po��czenie klienta z serwerem
    while (true)
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

    fd_set set;
    FD_ZERO(&set);
    FD_SET(tcpSocket, &set);

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // Sprawdzenie, czy istniej� dane gotowe do odczytu
    int result = select(0, &set, NULL, NULL, &timeout);
    if (result == SOCKET_ERROR) {
        std::cout << "B��d w funkcji select(): " << WSAGetLastError() << std::endl;
        return;
    }
    else if (result == 0) {
        return;
    }
    // Odebranie wiadomo�ci
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
            if (receivedString == "W")
            {
                std::cout << "zmiana na " << receivedString << std::endl;
                enemySnake.nextDirection = SnakeObject::Direction::Up;
            }
            if (receivedString == "A")
            {
                std::cout << "zmiana na " << receivedString << std::endl;
                enemySnake.nextDirection = SnakeObject::Direction::Left;
            }
            if (receivedString == "S")
            {
                std::cout << "zmiana na " << receivedString << std::endl;
                enemySnake.nextDirection = SnakeObject::Direction::Down;
            }
            if (receivedString == "D")
            {
                std::cout << "zmiana na " << receivedString << std::endl;
                enemySnake.nextDirection = SnakeObject::Direction::Right;
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
            std::cout << "problem z wys�aniem prosby o zakonczenie gry\n" << std::endl;
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
            std::cout << "problem z wys�aniem gotowosci\n" << std::endl;
        }
    }
}