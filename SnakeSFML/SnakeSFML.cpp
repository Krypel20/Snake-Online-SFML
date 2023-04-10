#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <Windows.h> //only for Sleep() in main()
#include <iostream>
#include <list>
#include <random>
#include <chrono>
#include <thread>

//GAME SETS6
const int windowWidth = 1200;
const int windowHeight = 800;
const int blockSize = 20;
const sf::Color backgroundColor = sf::Color::Black;
const sf::Color snakeColor = sf::Color::Green;
const sf::Color foodColor = sf::Color::Red;
sf::TcpSocket tcpSocket;
int LocalPoints;

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
    int score; //zmienna do przechowywania aktualnego wyniku
    int score2;

private:
    void handleInput();
    void update();
    void render();
    void generateFood();
    bool checkCollision();
    void sendReciveInfo();

    sf::RenderWindow window;
    std::list<SnakeSegment> snake;
    SnakeSegment head;
    sf::RectangleShape food;
    enum Direction { Up, Down, Left, Right } currentDirection;
    Direction nextDirection;
    sf::Font font; // czcionka do wyświetlania tekstu
    sf::Text scoreText; // tekst wyświetlający aktualny wynik
    sf::Text gameOverText; // tekst wyświetlający koniec gry i punkty graczy
};

SnakeGame::SnakeGame() : window(sf::VideoMode(windowWidth, windowHeight), "Snake Game - Piotr Krypel") {
    head.x = windowWidth / 2;
    head.y = windowHeight / 2;
    snake.push_front(head);

    currentDirection = Direction::Up;
    nextDirection = Direction::Up;
    generateFood();

    if (!font.loadFromFile("arial.ttf")) { // załaduj czcionkę z pliku arial.ttf
        std::cout << "Blad: nie udalo się zaladowac czcionki!" << std::endl;
    }

    score = 0;
    score2 = 0;
    scoreText.setFont(font);
    scoreText.setCharacterSize(22);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10, 10);
}

void SnakeGame::sendReciveInfo()
{
    ///* klasa Packet ułatwia wysyłanie wiadomosci do serwera dzięki automatycznej obsłudze konwersji pakietu sf::Packet 
    //na dane binarne które mogą być przesłane przez gniazdo. */
    //sf::Packet myScore; //wynik gracza
    //sf::Packet enemyScore; //wynik przeciwnika
    //myScore << score; //Przekonwertowanie wyniku (int) na wynik (Packet)
    std::size_t receivedSize;

    if (tcpSocket.send(&score, sizeof(int)) != sf::Socket::Done)
    {
        std::cout << "problem z przeslaniem wyniku gracza: " << score<<std::endl;
    }
    else { 
        tcpSocket.send(&score, sizeof(int));
        std::cout << "aktualny wynik gracza: " << score << std::endl;
    }


    if (tcpSocket.receive(&score2,sizeof(score2),receivedSize)!=sf::Socket::Done)
    {
        std::cout << "problem z odebraniem wyniku gracza, aktualny wynik przeciwnika: " << score2 << std::endl;
    }
    else { 
        tcpSocket.receive(&score2, sizeof(score2), receivedSize);
        std::cout<<"aktualny wynik przeciwnika: "<<score2<<std::endl;
    }
}

void SnakeGame::run() {
    sf::Clock clock;
    float deltaTime;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
        deltaTime = clock.restart().asSeconds();
        handleInput();
        update();
        render();
        sendReciveInfo();
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }
}

void SnakeGame::updateScore() 
{
    // Aktualizacja tekstu wyświetlającego aktualny wynik
    scoreText.setString("Twoje punkty: " + std::to_string(score)+ "\n Punkty przeciwnika: "+std::to_string(score2));
}

void SnakeGame::gameOver()
{
    gameOverText.setFont(font);
    gameOverText.setCharacterSize(40);
    gameOverText.setFillColor(sf::Color::White);
    gameOverText.setPosition(windowWidth/2-465,windowHeight/2);

    if (score > score2){
        gameOverText.setString("WYGRALES! Twoje punkty: " + std::to_string(score) + " Punkty przeciwnika: " + std::to_string(score2));
    }
    else if(score==score2){
        gameOverText.setString("REMIS! Twoje punkty: " + std::to_string(score) + " Punkty przeciwnika: " + std::to_string(score2));
    }
    else
    {
        gameOverText.setString("PRZEGRALES! Twoje punkty: " + std::to_string(score) + " Punkty przeciwnika: " + std::to_string(score2));
    }
}

void SnakeGame::handleInput() {

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

void SnakeGame::update() {
    // Aktualizacja kierunku na podstawie następnego kierunku
    currentDirection = nextDirection;
    window.draw(scoreText);
    updateScore();

    // Aktualizacja pozycji głowy węża na podstawie kierunku
    head = snake.front();
    switch (currentDirection) {
    case Direction::Up:
        head.y -= blockSize;
        break;
    case Direction::Down:
        head.y += blockSize;
        break;
    case Direction::Left:
        head.x -= blockSize;
        break;
    case Direction::Right:
        head.x += blockSize;
        break;
    }

    // Dodanie nowej głowy węża na początek listy
    snake.push_front(head);
    // Sprawdzenie kolizji z jedzeniem oraz z samym sobą
    if (head.x == food.getPosition().x && head.y == food.getPosition().y) {

        generateFood();
        score++; //dodanie punktów
        updateScore();
    }
    else {
        // Usunięcie ostatniego segmentu węża
        snake.pop_back();
    }
    if (checkCollision()) {
        // Zderzenie z samym sobą - koniec gry
        window.clear();
        gameOver();
    }
}

void SnakeGame::render() {
    window.clear(backgroundColor);

    // Rysowanie węża
    for (const auto& segment : snake) {
        sf::RectangleShape segmentShape(sf::Vector2f(blockSize, blockSize));
        segmentShape.setPosition(segment.x, segment.y);
        segmentShape.setFillColor(snakeColor);
        window.draw(segmentShape);
    }

    // Rysowanie jedzenia
    window.draw(food);
    window.draw(scoreText);
    window.draw(gameOverText);
    window.display();
}

void SnakeGame::generateFood() {

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

bool SnakeGame::checkCollision() {

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

reconnect:

    // Ustanawianie połączenia z serwerem
    sf::IpAddress serverIp = "192.168.1.20"; 
    int serverPort = 202012;

    if (tcpSocket.connect(serverIp, serverPort) != sf::Socket::Done) { // Obsługa błędu połączenia
   
        std::cout << "nie udalo polaczyc sie z serwerem...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1)); // program czeka 1 s
        goto reconnect;
    }


    // Potwierdzenie połączenia z serwerem
    std::cout << "Polaczenie udane!";
    std::string message = "Client coneccted!\n"; // Przykładowa wiadomość
    do
    {
        short t = 0;
        if (tcpSocket.send(message.c_str(), message.size() + 1) != sf::Socket::Done && t <= 15) {
            // Obsługa błędu wysyłania danych
            std::cout << "Connection error..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2)); // program czeka 1 s
            t++;
        }
        else if(tcpSocket.send(message.c_str(), message.size() + 1) == sf::Socket::Done){ 
            break; //wyjscie z petli
        }
        else { 
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
        return -1; // Zakończenie programu z kodem błędu
    }
    // Wyświetlenie otrzymanych danych jako test
    std::cout << "Otrzymane dane od serwera: " << recMsg << "\n"; 

    //inicjalizacja okna i silnika gry
    std::cout << "--------------------Rozpoczecie gry!--------------------\n";
    message = "START";
    tcpSocket.send(message.c_str(), message.size());
    SnakeGame game;
    game.run();
    /*tcpSocket.close();*/
    return 0;
}

//funkcje obslugujace przesyl danych klient <-> serwer
