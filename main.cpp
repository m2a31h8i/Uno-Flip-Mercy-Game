#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <random>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
using namespace std;

enum class Color { Red, Pink, Purple, Yellow, None };
enum class Value { One, Two, Three, Four, Five, Six, Seven, Eight, Nine, Reverse, DrawTwo, Block, DrawFour, ColorChange };

class Card {
public:
    Color color;
    Value value;
    Card(Color c, Value v) : color(c), value(v) {}
    void display() const {
        string colors[] = { "Red", "Pink", "Purple", "Yellow", "None" };
        string values[] = { "1","2","3","4","5","6","7","8","9","Reverse","DrawTwo","Block","DrawFour","ColorChange" };
        cout << colors[(int)color] << " " << values[(int)value];
    }
    bool canPlayOn(const Card& topCard) const {
        return color == topCard.color || value == topCard.value || value == Value::DrawFour || value == Value::ColorChange;
    }
};

class Deck {
public:
    vector<Card> cards;
    Deck() { initialize(); shuffle(); }

    void initialize() {
        cards.clear();
        int numColors = 4; // Red, Pink, Purple, Yellow
        for (int c = 0; c < numColors; ++c) {
            for (int v = 0; v <= 8; ++v) { // One to Nine
                cards.push_back(Card(static_cast<Color>(c), static_cast<Value>(v)));
                cards.push_back(Card(static_cast<Color>(c), static_cast<Value>(v))); // duplicate
            }
            // Special cards
            cards.push_back(Card(static_cast<Color>(c), Value::Reverse));
            cards.push_back(Card(static_cast<Color>(c), Value::DrawTwo));
            cards.push_back(Card(static_cast<Color>(c), Value::Block));
        }
        // Wild cards with Color None
        for(int i=0;i<4;i++)
            cards.push_back(Card(Color::None, Value::DrawFour));
        for(int i=0;i<2;i++)
            cards.push_back(Card(Color::None, Value::ColorChange));
    }

    void shuffle() {
        random_device rd;
        mt19937 g(rd());
        std::shuffle(cards.begin(), cards.end(), g);
    }

    Card draw() {
        Card c = cards.back();
        cards.pop_back();
        return c;
    }

    bool empty() const { return cards.empty(); }
};

class Player {
public:
    string name;
    vector<Card> hand;
    Player(const string& n) : name(n) {}
    virtual ~Player() {}
    void drawCard(Deck& deck, int count = 1) {
        for(int i=0;i<count;i++)
            hand.push_back(deck.draw());
    }
    void showHand() const {
        cout << name << "'s hand:\n";
        for(size_t i=0;i<hand.size();i++){
            cout << i+1 << ". "; hand[i].display(); cout << "\n";
        }
    }
    virtual int playTurn(const Card& topCard) = 0;
    virtual Color chooseColor() = 0;
};

class HumanPlayer : public Player {
public:
    HumanPlayer(const string& n) : Player(n) {}
    int playTurn(const Card& topCard) override {
        while(true) {
            showHand();
            cout << "Top card: "; topCard.display(); cout << "\n";
            cout << "Enter card number to play or 0 to draw: ";
            int choice;
            if(!(cin >> choice)) {
                cin.clear(); // clear invalid input
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Try again.\n";
                continue;
            }
            if(choice == 0) return -1; // draw
            if(choice < 1 || choice > (int)hand.size()) {
                cout << "Invalid card number. Try again.\n";
                continue;
            }
            if(hand[choice-1].canPlayOn(topCard)) return choice-1;
            cout << "Cannot play that card. Try again.\n";
        }
    }

    Color chooseColor() override {
        cout << "Choose a color (0=Red,1=Pink,2=Purple,3=Yellow): ";
        int c;
        if(!(cin >> c) || c < 0 || c > 5) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid choice. Try again.\n";
            return chooseColor();
        }
        return static_cast<Color>(c);
    }
};

class AIPlayer : public Player {
public:
    AIPlayer(const string& n) : Player(n) {}
    int playTurn(const Card& topCard) override {
        for(size_t i=0;i<hand.size();i++)
            if(hand[i].canPlayOn(topCard))
                return i;
        return -1;
    }

    Color chooseColor() override {
        int counts[4] = {0};
        for(auto &card : hand)
            if(card.color != Color::None && (int)card.color < 4) counts[(int)card.color]++;
        int maxIndex = 0;
        for(int i=1;i<4;i++) if(counts[i]>counts[maxIndex]) maxIndex=i;
        return static_cast<Color>(maxIndex);
    }
};

class Game {
private:
    Deck deck;
    deque<shared_ptr<Player>> players;
    Card topCard;
    int direction;
public:
    Game() : topCard(Color::Red, Value::One), direction(1) {}

    void setup() {
        players.push_back(make_shared<HumanPlayer>("You"));
        players.push_back(make_shared<AIPlayer>("AI-1"));
        players.push_back(make_shared<AIPlayer>("AI-2"));
        for(auto& p : players) p->drawCard(deck, 5);
        topCard = deck.draw();
    }

    void start() {
        int current = 0;
        while(true) {
            auto player = players[current];
            cout << "\n" << player->name << "'s turn.\n";
            int choice = player->playTurn(topCard);
            if(choice == -1) {
                cout << player->name << " draws a card.\n";
                player->drawCard(deck);
            } else {
                topCard = player->hand[choice];
                player->hand.erase(player->hand.begin() + choice);
                cout << player->name << " plays "; topCard.display(); cout << "\n";

                if(topCard.value == Value::DrawFour || topCard.value == Value::ColorChange) {
                    Color newColor = player->chooseColor();
                    topCard.color = newColor;
                    string colors[] = { "Red", "Pink", "Purple", "Yellow",};
                    cout << player->name << " changed color to " << colors[(int)newColor] << "\n";

                    if(topCard.value == Value::DrawFour) {
                        int next = (current + direction + players.size()) % players.size();
                        players[next]->drawCard(deck, 4);
                    }
                }

                if(player->hand.empty()) {
                    cout << player->name << " wins!\n";
                    break;
                }

                if(topCard.value == Value::Reverse) direction *= -1;
                if(topCard.value == Value::Block)
                    current = (current + direction + players.size()) % players.size();
                if(topCard.value == Value::DrawTwo) {
                    int next = (current + direction + players.size()) % players.size();
                    players[next]->drawCard(deck, 2);
                }
            }
            current = (current + direction + players.size()) % players.size();
        }
    }
};

int main() {
    Game game;
    game.setup();
    game.start();
    return 0;
}
