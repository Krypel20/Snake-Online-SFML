#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <list>
#include <random>
#include <chrono>
#include <thread>

sf::IpAddress serverIp = "83.27.209.62"; //ip serwera
int serverPort = 20202;

//GAME SETS
const int windowWidth = 1200;
const int windowHeight = 900;
const int blockSize = 20;
const int frameLimit = 55;
const sf::Color backgroundColor = sf::Color::Black;
const sf::Color snakeColor = sf::Color::Green;
const sf::Color foodColor = sf::Color::Red;
float deltaT;
sf::Clock gameClock;
sf::TcpSocket tcpSocket;
sf::RectangleShape snakeShape(sf::Vector2f(blockSize, blockSize)); //wąż
bool isGameOver;
bool gameStarted;

struct SnakeSegment {
    int x;
    int y;
};

class SnakeGame {
public:
    SnakeGame();
    void run();
    void updateScore();
    void gameOver();
    void gameStartProcedure();
    bool gameStart();
    int P1Score; //zmienna do przechowywania aktualnego wyniku
    int P2Score;
    bool isGamePaused;
    bool enemyGameOver;

private:
    void handleInput();
    void update();
    void gameInfo();
    void render();
    void generateFood();
    bool checkCollision();
    void receiveScore();
    void sendScore();
    void sendGameOver();
    void receiveGameOver();
    void printGameOver();
    void receiveDecision();
    void sendDecision();

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
    window.setFramerateLimit(frameLimit);
    head.x = windowWidth / 2;
    head.y = windowHeight / 2;
    snake.push_front(head);

    currentDirection = Direction::Up;
    nextDirection = Direction::Up;
    generateFood();

    if (!font.loadFromFile("arial.ttf")) { // załaduj czcionkę z pliku arial.ttf
        std::cout << "Blad: nie udalo się zaladowac czcionki!" << std::endl;
    }

    isGamePaused = false;
    enemyGameOver = false;
    P1Score = 0;
    P2Score = 0;
}

void SnakeGame::gameStartProcedure()
{
    gameStartText.setFont(font);
    gameStartText.setCharacterSize(60);
    gameStartText.setFillColor(sf::Color::White);
    gameStartText.setPosition(windowWidth / 2 - 465, windowHeight / 2);
    gameOverText.setString("Oczekiwanie na gotowość graczy...\nWciśnij Spacje by rozpocząć");
    window.draw(gameStartText);
    gameStartText.setCharacterSize(80);
    for (int i = 3; i >= 1; --i)
    {
        gameStartText.setString(std::to_string(i));
        sf::sleep(sf::seconds(1));
    }
    gameOverText.setString("");
}

bool SnakeGame::gameStart()
{
    return true;
}

void SnakeGame::run()
{
    while (window.isOpen())
    {
        sf::Event event;
        if (gameStart() == true)
        {
            deltaT = gameClock.restart().asSeconds();
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }
            if (isGameOver)
            {
                render();
                receiveScore();
                receiveGameOver();
                updateScore();
                printGameOver();
            }
            else {
                render();
                handleInput();
                update();
                receiveScore();
                updateScore();
                receiveGameOver();
                gameInfo();
            }
            if (isGamePaused && !isGameOver)
            {
                gameInfo();
            }
        }
    }
}

void SnakeGame::updateScore()
{
    // Aktualizacja tekstu wyświetlającego aktualny wynik
    scoreText.setFont(font);
    scoreText.setCharacterSize(22);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 10);
    scoreText.setString("Gracz: " + std::to_string(P1Score) + "\nPrzeciwnik: " + std::to_string(P2Score));
}

void SnakeGame::gameInfo()
{
    gameInfoText.setFont(font);
    gameInfoText.setCharacterSize(22);
    gameInfoText.setFillColor(sf::Color::White);
    gameInfoText.setPosition(10, 60);
    gameInfoText.setString("dt: " + std::to_string(deltaT));
}

void SnakeGame::printGameOver()
{
    gameOverText.setFont(font);
    gameOverText.setCharacterSize(40);
    gameOverText.setFillColor(sf::Color::Yellow);
    gameOverText.setPosition((windowWidth - windowHeight) / 2, windowHeight / 2);
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

void SnakeGame::gameOver()
{
    isGameOver = true;
    isGamePaused = true; //zatrzymanie gry
    printGameOver();
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

void SnakeGame::update()
{
    if (!isGamePaused)
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
                snake.push_back(newTail);
            }
        }
        else {
            // Usunięcie ostatniego segmentu węża?
            snake.pop_back();
        }
        if (checkCollision()) {
            // Zderzenie z samym sobą - koniec gry
            gameOver();
        }
    }
}
void SnakeGame::render()
{
    window.clear(backgroundColor);
    // Rysowanie węża
    for (const auto& segment : snake) {
        snakeShape.setPosition(segment.x, segment.y);
        snakeShape.setFillColor(snakeColor);
        window.draw(snakeShape);
    }

    // Rysowanie jedzenia
    window.draw(food);
    window.draw(scoreText);
    window.draw(gameInfoText);
    window.draw(gameOverText);
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


int main(int argc, char** argv)
{
Reconnect:
    // Ustanawianie połączenia z serwerem
    std::cout << "Laczenie z serwerem: " << serverIp << ":" << serverPort << std::endl;
    while (true)
    {
        if (tcpSocket.connect(serverIp, serverPort) == sf::Socket::Done)
        {
            std::cout << "Polaczenie udane...\n";
            break;
        }
        else
        {
            std::cout << "Nie udalo polaczyc sie z serwerem...\n";
            std::this_thread::sleep_for(std::chrono::seconds(1)); // program czeka 1 s
        }
    }

    std::string message = "test!\n"; // Pierwsza wiadomość potwierdzająca połączenie klienta z serwerem
    do
    {
        if (tcpSocket.send(message.c_str(), message.size() + 1) != sf::Socket::Done) {
            // Obsługa błędu wysyłania danych
            std::cout << "blad polaczenia...\n" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        else if (tcpSocket.send(message.c_str(), message.size() + 1) == sf::Socket::Done) {
            break; //wyjscie z petli
        }
        else {
            std::cout << "-------FATAL ERROR-------- \n" << std::endl;
            tcpSocket.disconnect();
            return -1; // Zakończenie programu
        }
    } while (tcpSocket.send(message.c_str(), message.size() + 1) != sf::Socket::Done);

    // Sprawdzenie czy klient moze odebrac dane
    char recMsg[512];
    std::size_t received;
    if (tcpSocket.receive(recMsg, sizeof(recMsg), received) != sf::Socket::Done) {
        // Obsługa błędu odbierania danych
        // np. wypisanie komunikatu o błędzie lub zakończenie gry
        std::cout << "Nie udało się odebrać danych od serwera.\n" << std::endl;
        tcpSocket.disconnect();
        return -1; // Zakończenie programu z kodem błędu
    }
    // Wyświetlenie otrzymanych danych jako test
    /*std::cout << "Otrzymane dane od serwera: " << recMsg << "\n";*/

    //inicjalizacja okna i silnika gry
    std::cout << "--------------------Rozpoczecie gry!--------------------\n";
    SnakeGame game;
    game.run();
    std::cout << "--------------------Gra sie zakonczyla!-----------------\n" << std::endl;
    tcpSocket.disconnect();
    return 0;
}


void SnakeGame::receiveScore()
{
    std::size_t receivedSize;
    tcpSocket.setBlocking(false); //!!!

    if (tcpSocket.receive(&P2Score, sizeof(int), receivedSize) == sf::Socket::Done)
    {
        std::cout << "Wynik przeciwnika: " << P2Score << std::endl;
    }
}

void SnakeGame::sendScore() //funkcja wymieniająca wyniki graczy z serwerem
{
    if (tcpSocket.send(&P1Score, sizeof(int)) == sf::Socket::Done)
    {
        std::cout << "Twoj wynik: " << P1Score << std::endl;
    }
}

void SnakeGame::receiveGameOver()
{
    tcpSocket.setBlocking(false);
    std::size_t receivedSize;
    std::string message;
    if (tcpSocket.receive(&message, sizeof(message), receivedSize) == sf::Socket::Done && message == "gameover")
    {
        std::cout << "Przeciwnik przegral: " << P2Score << std::endl;
    }
    return;
}

void SnakeGame::sendGameOver()
{
    if (isGameOver == true)
    {
        std::string message = "gameover";
        if (tcpSocket.send(&message, sizeof(message) != sf::Socket::Done))
        {
            std::cout << "problem z wysłaniem prosby o zakonczenie gry\n" << std::endl;
        }
        else { std::cout << "Gracz: " << message << std::endl; }
    }
}