#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <vector>
#include <iostream>
#include <algorithm>
#define BLOCKSIZE 300

void socket();
void runTcpServer(unsigned short port);
void runTcpClient(unsigned short port);

class box
{
public:
    box(int x, int y)
    {
        block.setSize(sf::Vector2f(BLOCKSIZE, BLOCKSIZE));
        block.setPosition(x, y);
        block.setOutlineColor(sf::Color::Black);
        block.setOutlineThickness(1.0);
        pos.x = x;
        pos.y = y;
        marked = false;
    }
    bool mark(sf::Texture &marking)
    {
        if (marked)
            return false;
        block.setTexture(&marking);
        marked = true;
        return true;
    }
    void draw(sf::RenderWindow &window)
    {
        window.draw(block);
    }

    sf::FloatRect getBounds()
    {
        return sf::FloatRect(pos.x, pos.y, BLOCKSIZE, BLOCKSIZE);
    }

    const sf::Texture *getTexture()
    {
        return block.getTexture();
    }

    void clearMark()
    {
        block.setTexture(NULL);
        marked = false;
    }

private:
    bool marked;
    sf::Vector2f pos;
    sf::RectangleShape block;
};

bool winCondition(std::vector<box *> boxes, sf::Texture &mark);

//find good place for these

int main()
{
    // Create the main window
    sf::RenderWindow window(sf::VideoMode(900, 900), "SFML window");

    std::vector<box *> boxes;
    boxes.push_back(new box(BLOCKSIZE * 0, BLOCKSIZE * 0));
    boxes.push_back(new box(BLOCKSIZE * 1, BLOCKSIZE * 0));
    boxes.push_back(new box(BLOCKSIZE * 2, BLOCKSIZE * 0));
    boxes.push_back(new box(BLOCKSIZE * 0, BLOCKSIZE * 1));
    boxes.push_back(new box(BLOCKSIZE * 1, BLOCKSIZE * 1));
    boxes.push_back(new box(BLOCKSIZE * 2, BLOCKSIZE * 1));
    boxes.push_back(new box(BLOCKSIZE * 0, BLOCKSIZE * 2));
    boxes.push_back(new box(BLOCKSIZE * 1, BLOCKSIZE * 2));
    boxes.push_back(new box(BLOCKSIZE * 2, BLOCKSIZE * 2));

    char option{'j'};
    sf::Texture marking;
    sf::Texture oponentMarking;

    sf::TcpSocket client; // TO CLIENT
    sf::TcpSocket socket; // TO HOST

    //start menu variables
    sf::RectangleShape joinButton(sf::Vector2f(200, 100));
    sf::RectangleShape hostButton(sf::Vector2f(200, 100));
    sf::Texture joinButtonTexture;
    sf::Texture hostButtonTexture;
    joinButtonTexture.loadFromFile("resources/join-button-texture.png");
    hostButtonTexture.loadFromFile("resources/host-button-texture.png");
    bool firstToMove;
    bool wasPressed = false;
    //end of start menu variables

    //end menu variables
    sf::Font endTextFont;
    if (!endTextFont.loadFromFile("resources/inter.ttf"))
    {
        std::cerr << "Could not load font\n";
    }
    sf::Text endText;
    endText.setFont(endTextFont);
    endText.setCharacterSize(24);
    endText.setStyle(sf::Text::Bold);
    sf::RectangleShape replayButton(sf::Vector2f(200, 100));
    sf::Texture replayButtonTexture;
    replayButtonTexture.loadFromFile("resources/replay-button-texture.png");
    replayButton.setTexture(&replayButtonTexture);
    //end of end menu variables
    int state = 0;
    int markedBoxes = 0;
    enum State
    {
        start,
        play,
        wait,
        win,
        lose,
        end,
        playAgain,
    };

    // Start the game loop
    while (window.isOpen())
    {
        // Process events
        sf::Event event;
        while (window.pollEvent(event))
        {
            // Close window: exit
            if (event.type == sf::Event::Closed)
                window.close();
        }

        if (!sf::Mouse::isButtonPressed(sf::Mouse::Left))
            wasPressed = false;
        switch (state)
        {
        case start:
        {
            joinButton.setPosition(100, 300);
            hostButton.setPosition(600, 300);
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                sf::FloatRect joinBounds = joinButton.getGlobalBounds();
                sf::FloatRect hostBounds = hostButton.getGlobalBounds();

                if (joinBounds.contains(mouse))
                {
                    marking.loadFromFile("resources/circle-texture.png");
                    oponentMarking.loadFromFile("resources/cross-texture.png");
                    int port;
                    std::string ip;
                    std::cout << "IP ADRESS OF HOST: ";
                    std::cin >> ip;
                    std::cout << std::endl;
                    std::cout << "PORT: ";
                    std::cin >> port;
                    std::cout << std::endl;
                    sf::Socket::Status status = socket.connect(ip, port);

                    if (status != sf::Socket::Done)
                    {
                        std::cerr << "Could not connect to server on ip " << ip << std::endl;
                    }
                    socket.setBlocking(false);

                    state = wait;
                    firstToMove = false;
                }
                else if (hostBounds.contains(mouse))
                {
                    marking.loadFromFile("resources/cross-texture.png");
                    oponentMarking.loadFromFile("resources/circle-texture.png");
                    sf::TcpListener listener;
                    int port;
                    std::cout << "PORT TO LISTEN ON: ";
                    std::cin >> port;
                    std::cout << std::endl;

                    if (listener.listen(port) != sf::Socket::Done)
                    {
                        std::cerr << "Could not listen on port 9001" << port << std::endl;
                    }

                    if (listener.accept(client) != sf::Socket::Done)
                    {
                        std::cerr << "Could not accept client on port " << port << std::endl;
                    }
                    client.setBlocking(false);
                    option = 'h';
                    state = play;
                    firstToMove = true;
                }
            }
            joinButton.setTexture(&joinButtonTexture);
            hostButton.setTexture(&hostButtonTexture);
            window.clear();
            window.draw(joinButton);
            window.draw(hostButton);
            window.display();
            break;
        }
        case play:
        {
            if (!wasPressed && sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                // transform the mouse position from window coordinates to world coordinates
                sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                for (size_t i = 0; i < boxes.size(); i++)
                {
                    sf::FloatRect bounds = boxes[i]->getBounds();

                    if (bounds.contains(mouse))
                    {
                        if (boxes[i]->mark(marking))
                        {
                            sf::Packet packet;
                            if (winCondition(boxes, marking))
                            {
                                packet << "LOSER!";
                                state = end;
                                endText.setFillColor(sf::Color::Green);
                                endText.setString("You win!");
                            }
                            packet << std::to_string(bounds.left) + " " + std::to_string(bounds.top);

                            if (option == 'h')
                                client.send(packet);
                            else
                                socket.send(packet);
                            if (state != end)
                            {
                                state = wait;
                                if (++markedBoxes == 9)
                                {
                                    endText.setString("TIE!");
                                    endText.setFillColor(sf::Color::Blue);
                                    state = end;
                                }
                            }
                        }
                    }
                }
            }
            window.clear();
            for (size_t i = 0; i < boxes.size(); i++)
                boxes[i]->draw(window);
            window.display();
        }
        break;
        case wait:
        {
            sf::Packet data;
            if (option == 'h')
                client.receive(data);
            else
                socket.receive(data);
            std::string s{""};
            data >> s;
            if (s != "")
            {
                int x, y;
                if (s == "LOSER!")
                {
                    state = end;
                    data >> s;
                    endText.setFillColor(sf::Color::Red);
                    endText.setString("You lose!");
                }

                x = stoi(s.substr(0, 3));
                if (x == 0)
                    y = stoi(s.substr(9, 3));
                else
                    y = stoi(s.substr(11, 3));

                for (size_t i = 0; i < boxes.size(); i++)
                {
                    sf::FloatRect bounds = boxes[i]->getBounds();
                    if (bounds.left == x && bounds.top == y)
                        boxes[i]->mark(oponentMarking);
                }
                if (state != end)
                {
                    state = play;
                    if (++markedBoxes == 9)
                    {
                        endText.setString("TIE!");
                        endText.setFillColor(sf::Color::Blue);
                        state = end;
                    }
                }
            }
            window.clear();
            for (size_t i = 0; i < boxes.size(); i++)
                boxes[i]->draw(window);
            window.display();

            break;
        }
        case end:
        {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                // transform the mouse position from window coordinates to world coordinates
                sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                sf::FloatRect bounds = replayButton.getGlobalBounds();
                if (bounds.contains(mouse)) // send disconnect if disconnect OOKOKOK
                {
                    endText.setString("Waiting for other player...");
                    endText.setFillColor(sf::Color::White);
                    state = playAgain;
                    sf::Packet replayPacket;
                    replayPacket << "yes";
                    if (option == 'h')
                        client.send(replayPacket);
                    else
                        socket.send(replayPacket);
                }
            }
            replayButton.setPosition(450, 450);
            endText.setPosition(450, 100);
            window.clear();
            window.draw(endText);
            window.draw(replayButton);
            window.display();
            break;
        }
        case playAgain:
        {
            sf::Packet replayPacket;
            if (option == 'h')
                client.receive(replayPacket);
            else
                socket.receive(replayPacket);
            std::string data;
            replayPacket >> data;
            if (data == "yes")
            {
                if (firstToMove)
                    state = wait;
                else
                    state = play;
                firstToMove = !firstToMove;
                for (size_t i = 0; i < boxes.size(); i++)
                {
                    boxes[i]->clearMark();
                }
                wasPressed = true;
                markedBoxes = 0;
            }
            endText.setPosition(300, 100);
            window.clear();
            window.draw(endText);
            window.display();
            break;
        }
        default:
            break;
        }
    }
}
const int wins[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};

bool winCondition(std::vector<box *> boxes, sf::Texture &mark)
{
    using namespace std;
    vector<int> markedBoxes{};
    for (size_t i = 0; i < boxes.size(); i++)
    {
        if (boxes[i]->getTexture() == &mark)
            markedBoxes.push_back(i);
    }
    auto begin = markedBoxes.begin();
    auto end = markedBoxes.end();
    for (int i = 0; i < 8; i++)
    {
        if (std::find(begin, end, wins[i][0]) != end && std::find(begin, end, wins[i][1]) != end && std::find(begin, end, wins[i][2]) != end)
        {
            return true;
        }
    }
    return false;
}
