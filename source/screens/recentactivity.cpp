#include <ctime>
#include "listitem_recentactivity.hpp"
#include "screenmanager.hpp"
#include "SDLHelper.hpp"
#include "utils.hpp"

namespace Screen {
    RecentActivity::RecentActivity(User * u) : Screen::Screen() {
        this->list = new UI::List(&ScreenManager::getInstance()->touch_active, 400, 110, 850, 515);
        this->playdata = new PlayData();
        this->user = u;
        this->total_hours = nullptr;

        // Set controls
        this->controls->add(KEY_A, "OK", 0);
        if (!this->is_mypage) {
            this->controls->add(KEY_PLUS, "Exit", 1);
        }
        this->controls->add(KEY_X, "View by", 2);

        // Set active element
        this->active_element = (int)ActiveElement::SideMenu;
        this->menu->setActive(true);
        this->list->setActive(false);
    }

    void RecentActivity::generateList(u64 start_ts, u64 end_ts) {
        // Delete old list
        if (this->list != nullptr) {
            delete this->list;
        }
        this->list = new UI::List(&ScreenManager::getInstance()->touch_active, 400, 110, 850, 515);

        // Get play statistics for range
        std::vector<u64> ids = this->playdata->getLoggedTitleIDs();
        u32 total_mins = 0;
        for (size_t i = 0; i < ids.size(); i++) {
            RecentPlayStatistics * stats = this->playdata->getRecentStatisticsForUser(ids[i], start_ts, end_ts, this->user->getID());
            if (stats->playtime > 0) {
                this->list->addItem(new UI::ListItem::RecentActivity(stats));
            }
            total_mins += stats->playtime;
            delete stats;
        }

        // Generate total time texture
        std::string str = "Total Playtime: ";
        str += Utils::formatPlaytime(total_mins, " and ");
        this->total_hours = SDLHelper::renderText(str.c_str(), BODY_FONT_SIZE);
    }

    void RecentActivity::event(SDL_Event e){
        switch (e.type) {
            // Button pressed
            case SDL_JOYBUTTONDOWN:
                if (e.jbutton.which == 0 || e.jbutton.which == 99) {
                    // Plus exits app
                    if (e.jbutton.button == Utils::key_map[KEY_PLUS]) {
                        if (!this->is_mypage) {
                            ScreenManager::getInstance()->stopLoop();
                        }

                    // Left and right will swap active element
                    } else if (e.jbutton.button == Utils::key_map[KEY_DLEFT]) {
                        this->active_element = (int)ActiveElement::SideMenu;
                        this->menu->setActive(true);
                        this->list->setActive(false);
                    } else if (e.jbutton.button == Utils::key_map[KEY_DRIGHT]) {
                        this->active_element = (int)ActiveElement::List;
                        this->menu->setActive(false);
                        this->list->setActive(true);

                    // X brings up period to view
                    } else if (e.jbutton.button == Utils::key_map[KEY_X]) {
                        std::vector<std::string> v;
                        v.push_back("By Day");
                        v.push_back("By Month");
                        v.push_back("By Year");
                        ScreenManager::getInstance()->createSelection("View Period", v, [this](int c) {
                            switch (c) {
                                case -1:
                                    // Do nothing if nothing selected
                                    break;

                                case 0: {
                                    // Get current day
                                    time_t now = std::time(nullptr);
                                    struct tm * start = std::localtime(&now);
                                    start->tm_hour = 0;
                                    start->tm_min = 0;
                                    start->tm_sec = 0;
                                    time_t ts = std::mktime(start);
                                    this->generateList(ts, now);
                                    break;
                                }

                                case 1: {
                                    // Get current month
                                    time_t now = std::time(nullptr);
                                    struct tm * start = std::localtime(&now);
                                    start->tm_mday = 1;
                                    start->tm_hour = 0;
                                    start->tm_min = 0;
                                    start->tm_sec = 0;
                                    time_t ts = std::mktime(start);
                                    this->generateList(ts, now);
                                    break;
                                }

                                case 2: {
                                    // Get current year
                                    time_t now = std::time(nullptr);
                                    struct tm * start = std::localtime(&now);
                                    start->tm_mon = 1;
                                    start->tm_mday = 1;
                                    start->tm_hour = 0;
                                    start->tm_min = 0;
                                    start->tm_sec = 0;
                                    time_t ts = std::mktime(start);
                                    this->generateList(ts, now);
                                    break;
                                }
                            }
                        });

                    // All other buttons get handled by active element
                    } else {
                        switch (this->active_element) {
                            case (int)ActiveElement::SideMenu:
                                this->menu->handleButton(e.jbutton.button, e.jbutton.state);
                                break;

                            case (int)ActiveElement::List:
                                this->list->handleButton(e.jbutton.button, e.jbutton.state);
                                break;
                        }
                    }
                }
                break;

            case SDL_JOYBUTTONUP:
                if (e.jbutton.which == 0 || e.jbutton.which == 99) {
                    switch (this->active_element) {
                        case (int)ActiveElement::SideMenu:
                            this->menu->handleButton(e.jbutton.button, e.jbutton.state);
                            break;

                        case (int)ActiveElement::List:
                            this->list->handleButton(e.jbutton.button, e.jbutton.state);
                            break;
                    }
                }
                break;

            // Touch (pressed)
            case SDL_FINGERDOWN: {
                float x = WIDTH * e.tfinger.x;
                float y = HEIGHT * e.tfinger.y;

                // Pressed within list
                if (x >= this->list->getX() && x <= this->list->getX() + this->list->getW() && y >= this->list->getY() && y <= this->list->getY() + this->list->getH()) {
                    this->list->touched(e.type, x, y);
                    this->active_element = (int)ActiveElement::List;
                    this->menu->setActive(false);
                    this->list->setActive(true);

                // Pressed within menu
                } else if (x >= this->menu->getX() && x <= this->menu->getX() + this->menu->getW() && y >= this->menu->getY() && y <= this->menu->getY() + this->menu->getH()) {
                    this->menu->touched(e.type, x, y);
                    this->active_element = (int)ActiveElement::SideMenu;
                    this->menu->setActive(true);
                    this->list->setActive(false);

                // Pass event to controls object if below bottom line
                } else if (y > 647) {
                    this->controls->touched(e.type, x, y);
                }
                break;
            }

            // Touch (moved)
            case SDL_FINGERMOTION: {
                float x = WIDTH * e.tfinger.x;
                float y = HEIGHT * e.tfinger.y;
                float dx = WIDTH * e.tfinger.dx;
                float dy = HEIGHT * e.tfinger.dy;

                // List scrolling overrides any other actions
                if (this->list->isTouched()) {
                    this->list->touched(e.type, x, y, dx, dy);

                // Moved after being in menu
                } else if ((x-dx) >= this->menu->getX() && (x-dx) <= this->menu->getX() + this->menu->getW() && (y-dy) >= this->menu->getY() && (y-dy) <= this->menu->getY() + this->menu->getH()) {
                    this->menu->touched(e.type, x, y);

                } else {
                    // Pass event to controls object if was below or originally below line
                    if (y > 647 || (HEIGHT * (e.tfinger.y - e.tfinger.dy)) > 647) {
                        this->controls->touched(e.type, x, y);
                    }
                }
                break;
            }

            // Touch (released)
            case SDL_FINGERUP: {
                float x = WIDTH * e.tfinger.x;
                float y = HEIGHT * e.tfinger.y;

                if (this->list->isTouched()) {
                    this->list->touched(e.type, x, y, (WIDTH * e.tfinger.dx), (HEIGHT * e.tfinger.dy));

                // Released within menu
                } else if (x >= this->menu->getX() && x <= this->menu->getX() + this->menu->getW() && y >= this->menu->getY() && y <= this->menu->getY() + this->menu->getH()) {
                    this->menu->touched(e.type, x, y);

                } else {
                    // Pass event to controls object if below bottom line
                    if (y > 647) {
                        this->controls->touched(e.type, x, y);
                    }
                }
                break;
            }
        }
    }

    void RecentActivity::update(uint32_t dt) {
        Screen::update(dt);
        this->menu->update(dt);

        if (ScreenManager::getInstance()->touch_active) {
            this->controls->disable(KEY_A);
        } else {
            this->controls->enable(KEY_A);
        }
        this->list->update(dt);

        // Change screen if menu selected
        switch (this->menu->getSelected()) {
            // Recent Activity (this one so do nothing)
            case 0:
                break;
            // All Activity
            case 1:
                ScreenManager::getInstance()->popScreen();
                break;
            // Settings
            case 2:
                ScreenManager::getInstance()->setScreen(new Settings(this->user));
                break;
        }
    }

    void RecentActivity::draw() {
        // Clear screen (draw background)
        SDLHelper::setColour(this->theme->getBG());
        SDLHelper::clearScreen();

        // Draw menu
        this->menu->draw();

        // Different shade behind list
        SDLHelper::setColour(this->theme->getAltBG());
        SDLHelper::drawRect(400, 88, 850, 560);

        // Draw list of items
        this->list->draw();

        // Draw over list to hide scrolling
        SDLHelper::setColour(this->theme->getBG());
        SDLHelper::drawRect(430, 0, 780, 87);
        SDLHelper::drawRect(430, 648, 780, 72);

        // Draw top and bottom lines
        SDLHelper::setColour(this->theme->getFG());
        SDLHelper::drawRect(30, 87, 1220, 1);
        SDLHelper::drawRect(30, 647, 1220, 1);

        // Draw player icon
        SDLHelper::drawTexture(this->user->getImage(), 65, 14, 60, 60);

        // Print heading
        std::string str = this->user->getUsername() + "'s Activity";
        SDLHelper::drawText(str.c_str(), this->theme->getText(), 150, 44 - (HEADING_FONT_SIZE/2), HEADING_FONT_SIZE);

        // Print total hours
        int tw, th;
        SDLHelper::getDimensions(this->total_hours, &tw, &th);
        SDLHelper::drawTexture(this->total_hours, this->theme->getMutedText(), 1215 - tw, 44 - th/2);

        // Draw controls
        this->controls->draw();
    }

    RecentActivity::~RecentActivity() {
        delete this->list;
        delete this->playdata;
    }
}