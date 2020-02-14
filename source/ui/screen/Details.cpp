#include "Details.hpp"
#include "ListSession.hpp"
#include "Utils.hpp"

// Values for summary appearance
#define SUMMARY_BOX_HEIGHT 60
#define SUMMARY_FONT_SIZE 21

namespace Screen {
    Details::Details(Main::Application * a) {
        this->app = a;

        // Create static elements
        Aether::Rectangle * r = new Aether::Rectangle(890, 88, 360, 559);
        r->setColour(this->app->theme()->altBG());
        this->addElement(r);
        r = new Aether::Rectangle(30, 87, 1220, 1);
        r->setColour(this->app->theme()->fg());
        this->addElement(r);
        r = new Aether::Rectangle(30, 647, 1220, 1);
        r->setColour(this->app->theme()->fg());
        this->addElement(r);
        Aether::Controls * c = new Aether::Controls();
        c->addItem(new Aether::ControlItem(Aether::Button::A, "OK"));
        c->addItem(new Aether::ControlItem(Aether::Button::B, "Back"));
        c->addItem(new Aether::ControlItem(Aether::Button::X, "View Date"));
        c->addItem(new Aether::ControlItem(Aether::Button::Y, "View By"));
        c->setColour(this->app->theme()->text());
        this->addElement(c);

        Aether::Text * t = new Aether::Text(1070, 120, "All Time Activity", 24);
        t->setColour(this->app->theme()->text());
        t->setX(t->x() - t->w()/2);
        this->addElement(t);
        t = new Aether::Text(1070, 190, "Play Time", 22);
        t->setColour(this->app->theme()->text());
        t->setX(t->x() - t->w()/2);
        this->addElement(t);
        t = new Aether::Text(1070, 280, "Average Play Time", 22);
        t->setColour(this->app->theme()->text());
        t->setX(t->x() - t->w()/2);
        this->addElement(t);
        t = new Aether::Text(1070, 370, "Times Played", 22);
        t->setColour(this->app->theme()->text());
        t->setX(t->x() - t->w()/2);
        this->addElement(t);
        t = new Aether::Text(1070, 460, "First Played", 22);
        t->setColour(this->app->theme()->text());
        t->setX(t->x() - t->w()/2);
        this->addElement(t);
        t = new Aether::Text(1070, 550, "Last Played", 22);
        t->setColour(this->app->theme()->text());
        t->setX(t->x() - t->w()/2);
        this->addElement(t);

        // Add key callbacks
        this->onButtonPress(Aether::Button::B, [this](){
            this->app->setScreen(Main::ScreenID::AllActivity);
        });
        this->onButtonPress(Aether::Button::X, [this](){
            this->app->createDatePicker();
        });
        this->onButtonPress(Aether::Button::Y, [this](){
            this->app->createPeriodPicker();
        });
        this->onButtonPress(Aether::Button::R, [this](){
            this->app->increaseDate();
        });
        this->onButtonPress(Aether::Button::L, [this](){
            this->app->decreaseDate();
        });
        this->onButtonPress(Aether::Button::ZR, [this](){
            this->app->setHoldDelay(30);
        });
        this->onButtonRelease(Aether::Button::ZR, [this](){
            this->app->setHoldDelay(100);
        });
        this->onButtonPress(Aether::Button::ZL, [this](){
            this->app->setHoldDelay(30);
        });
        this->onButtonRelease(Aether::Button::ZL, [this](){
            this->app->setHoldDelay(100);
        });
    }

    void Details::update(uint32_t dt) {
        if (this->app->timeChanged()) {
            this->list->setFocussed(this->header);
            this->updateGraph();
            this->updateSessions();
        }
        Screen::update(dt);
    }

    void Details::updateGraph() {
        // Setup graph columns + labels
        for (unsigned int i = 0; i < this->graph->entries(); i++) {
            this->graph->setLabel(i, "");
        }
        std::string heading = "Activity for ";
        struct tm tm = this->app->time();
        switch (this->app->viewPeriod()) {
            case ViewPeriod::Day:
                heading += Utils::Time::tmToString(tm, true, true, !(tm.tm_year == Utils::Time::getTmForCurrentTime().tm_year));
                this->graph->setFontSize(12);
                this->graph->setMaximumValue(60);
                this->graph->setYSteps(6);
                this->graph->setValuePrecision(0);
                this->graph->setNumberOfEntries(24);
                this->graph->setLabel(0, "12am");
                this->graph->setLabel(3, "3am");
                this->graph->setLabel(6, "6am");
                this->graph->setLabel(9, "9am");
                this->graph->setLabel(12, "12pm");
                this->graph->setLabel(15, "3pm");
                this->graph->setLabel(18, "6pm");
                this->graph->setLabel(21, "9pm");
                break;

            case ViewPeriod::Month: {
                heading += Utils::Time::tmToString(tm, false, true, true);
                unsigned int c = Utils::Time::tmGetDaysInMonth(tm);
                this->graph->setFontSize(12);
                this->graph->setValuePrecision(1);
                this->graph->setNumberOfEntries(c);
                for (unsigned int i = 0; i < c; i+=3) {
                    this->graph->setLabel(i, std::to_string(i + 1) + Utils::Time::getDateSuffix(i + 1));
                }
                break;
            }

            case ViewPeriod::Year:
                heading += Utils::Time::tmToString(tm, false, false, true);
                this->graph->setFontSize(14);
                this->graph->setValuePrecision(1);
                this->graph->setNumberOfEntries(12);
                for (int i = 0; i < 12; i++) {
                    this->graph->setLabel(i, Utils::Time::getShortMonthString(i));
                }
                break;
        }

        // Read play time and set graph values
        struct tm t = tm;
        unsigned int totalSecs = 0;
        switch (this->app->viewPeriod()) {
            case ViewPeriod::Day: {
                t.tm_min = 0;
                t.tm_sec = 0;
                struct tm e = t;
                e.tm_min = 59;
                e.tm_sec = 59;
                for (size_t i = 0; i < this->graph->entries(); i++) {
                    t.tm_hour = i;
                    e.tm_hour = i;
                    NX::RecentPlayStatistics * s = this->app->playdata()->getRecentStatisticsForUser(this->app->activeTitle()->titleID(), Utils::Time::getTimeT(t), Utils::Time::getTimeT(e), this->app->activeUser()->ID());
                    totalSecs += s->playtime;
                    double val = s->playtime/60.0;
                    this->graph->setValue(i, val);
                    delete s;
                }
                break;
            }

            case ViewPeriod::Month: {
                t.tm_hour = 0;
                t.tm_min = 0;
                t.tm_sec = 0;
                struct tm e = t;
                e.tm_hour = 23;
                e.tm_min = 59;
                e.tm_sec = 59;
                unsigned int max = 0;
                for (size_t i = 0; i < this->graph->entries(); i++) {
                    t.tm_mday = i + 1;
                    e.tm_mday = i + 1;
                    NX::RecentPlayStatistics * s = this->app->playdata()->getRecentStatisticsForUser(this->app->activeTitle()->titleID(), Utils::Time::getTimeT(t), Utils::Time::getTimeT(e), this->app->activeUser()->ID());
                    totalSecs += s->playtime;
                    if (s->playtime > max) {
                        max = s->playtime;
                    }
                    double val = s->playtime/60/60.0;
                    this->graph->setValue(i, val);
                    delete s;
                }
                max /= 60.0;
                max /= 60.0;
                if (max <= 2) {
                    this->graph->setMaximumValue(max + 2 - max%2);
                    this->graph->setYSteps(2);
                } else {
                    this->graph->setMaximumValue(max + 5 - max%5);
                    this->graph->setYSteps(5);
                }
                break;
            }

            case ViewPeriod::Year: {
                t.tm_mday = 1;
                t.tm_hour = 0;
                t.tm_min = 0;
                t.tm_sec = 0;
                struct tm e = t;
                e.tm_hour = 23;
                e.tm_min = 59;
                e.tm_sec = 59;
                unsigned int max = 0;
                for (size_t i = 0; i < this->graph->entries(); i++) {
                    t.tm_mon = i;
                    e.tm_mon = i;
                    e.tm_mday = Utils::Time::tmGetDaysInMonth(t);
                    NX::RecentPlayStatistics * s = this->app->playdata()->getRecentStatisticsForUser(this->app->activeTitle()->titleID(), Utils::Time::getTimeT(t), Utils::Time::getTimeT(e), this->app->activeUser()->ID());
                    totalSecs += s->playtime;
                    if (s->playtime > max) {
                        max = s->playtime;
                    }
                    double val = s->playtime/60/60.0;
                    this->graph->setValue(i, val);
                    delete s;
                }
                max /= 60.0;
                max /= 60.0;
                if (max <= 2) {
                    this->graph->setMaximumValue(max + 2 - max%2);
                    this->graph->setYSteps(2);
                } else {
                    this->graph->setMaximumValue(max + 5 - max%5);
                    this->graph->setYSteps(5);
                }
                break;
            }
        }

        // Set headings etc...
        this->graphHeading->setString(heading);
        this->graphHeading->setX(this->header->x() + (this->header->w() - this->graphHeading->w())/2);
        switch (this->app->viewPeriod()) {
            case ViewPeriod::Day:
                this->graphSubheading->setString("Play Time (in minutes)");
                break;

            case ViewPeriod::Month:
            case ViewPeriod::Year:
                this->graphSubheading->setString("Play Time (in hours)");
                break;
        }

        this->graphSubheading->setX(this->header->x() + (this->header->w() - this->graphSubheading->w())/2);
        if (totalSecs == 0) {
            this->graphTotalSub->setString("0 seconds");
        } else {
            this->graphTotalSub->setString(Utils::Time::playtimeToString(totalSecs, ", "));
        }

        int w = this->graphTotal->w() + this->graphTotalSub->w();
        this->graphTotal->setX(this->header->x());
        this->graphTotalSub->setX(this->graphTotal->x() + this->graphTotal->w());
        this->graphTotal->setX(this->graphTotal->x() + (this->header->w() - w)/2);
        this->graphTotalSub->setX(this->graphTotalSub->x() + (this->header->w() - w)/2);
    }

    void Details::updateSessions() {
        // Remove current sessions
        this->list->removeFollowingElements(this->topElm);

        // Get relevant play stats
        NX::PlayStatistics * ps = this->app->playdata()->getStatisticsForUser(this->app->activeTitle()->titleID(), this->app->activeUser()->ID());
        std::vector<NX::PlaySession> stats = this->app->playdata()->getPlaySessionsForUser(this->app->activeTitle()->titleID(), this->app->activeUser()->ID());

        // Get start and end timestamps for date
        char c = ' ';
        switch (this->app->viewPeriod()) {
            case ViewPeriod::Day:
                c = 'D';
                break;

            case ViewPeriod::Month:
                c = 'M';
                break;

            case ViewPeriod::Year:
                c = 'Y';
                break;
        }
        unsigned int s = Utils::Time::getTimeT(this->app->time());
        unsigned int e = Utils::Time::getTimeT(Utils::Time::increaseTm(this->app->time(), c));

        // Add sessions to list
        for (size_t i = 0; i < stats.size(); i++) {
            // Only add session if start or end is within the current time period
            if (stats[i].startTimestamp >= e || stats[i].endTimestamp < s) {
                continue;
            }

            // Create element
            CustomElm::ListSession * ls = new CustomElm::ListSession();
            ls->setLineColour(this->app->theme()->mutedLine());
            ls->setPercentageColour(this->app->theme()->mutedText());
            ls->setPlaytimeColour(this->app->theme()->accent());
            ls->setTimeColour(this->app->theme()->text());
            NX::PlaySession ses = stats[i];
            ls->setCallback([this, ses](){
                this->setupSessionBreakdown(ses);
            });

            // Set timestamp string
            std::string str = "";
            unsigned int playtime = stats[i].playtime;
            struct tm sTm = Utils::Time::getTm(stats[i].startTimestamp);
            struct tm eTm = Utils::Time::getTm(stats[i].endTimestamp);
            struct tm nTm = Utils::Time::getTmForCurrentTime();
            bool d = this->app->viewPeriod() != ViewPeriod::Day;
            bool m = this->app->viewPeriod() == ViewPeriod::Year;
            bool y = this->app->time().tm_year != nTm.tm_year;
            // If started before range set start as start of range
            if (stats[i].startTimestamp < s) {
                int hr = eTm.tm_hour;
                std::string str = Utils::Time::tmToString(this->app->time(), d, m, y) + ((str.length() == 0) ? "" : " ") + "12:00am - ";
                str += Utils::Time::tmToString(eTm, d, m, y) + std::to_string((hr > 12) ? hr - 12 : hr) + ":" + ((eTm.tm_min < 10) ? "0" : "") + std::to_string(eTm.tm_min) + ((eTm.tm_hour < 12) ? "am" : "pm") + "*";
                ls->setTimeString(str);

                NX::RecentPlayStatistics * rps = this->app->playdata()->getRecentStatisticsForUser(this->app->activeTitle()->titleID(), s, stats[i].endTimestamp, this->app->activeUser()->ID());
                ls->setPlaytimeString(Utils::Time::playtimeToString(rps->playtime, " and "));
                playtime = rps->playtime;
                delete rps;

            // If finished after range set end as end of range
            } else if (stats[i].endTimestamp >= e) {
                int hr = sTm.tm_hour;
                std::string str = Utils::Time::tmToString(sTm, d, m, y) + std::to_string((hr > 12) ? hr - 12 : hr) + ":" + ((sTm.tm_min < 10) ? "0" : "") + std::to_string(sTm.tm_min) + ((sTm.tm_hour < 12) ? "am" : "pm") + " - ";
                str += Utils::Time::tmToString(Utils::Time::increaseTm(this->app->time(), c), d, m, y) + " 11:59pm*";
                ls->setTimeString(str);

                NX::RecentPlayStatistics * rps = this->app->playdata()->getRecentStatisticsForUser(this->app->activeTitle()->titleID(), stats[i].startTimestamp, e, this->app->activeUser()->ID());
                ls->setPlaytimeString(Utils::Time::playtimeToString(rps->playtime, " and "));
                playtime = rps->playtime;
                delete rps;

            // Session starts before and finishes after
            } else if (stats[i].startTimestamp < s && stats[i].endTimestamp >= e) {
                std::string str = Utils::Time::tmToString(this->app->time(), d, m, y) + ((str.length() == 0) ? "" : " ") + "12:00am - " + Utils::Time::tmToString(Utils::Time::increaseTm(this->app->time(), c), d, m, y) + " 11:59pm*";
                ls->setTimeString(str);

                NX::RecentPlayStatistics * rps = this->app->playdata()->getRecentStatisticsForUser(this->app->activeTitle()->titleID(), s, e, this->app->activeUser()->ID());
                ls->setPlaytimeString(Utils::Time::playtimeToString(rps->playtime, " and "));
                playtime = rps->playtime;
                delete rps;

            // Session is within range
            } else {
                int hr = sTm.tm_hour;
                std::string str = Utils::Time::tmToString(sTm, d, m, y) + std::to_string((hr > 12) ? hr - 12 : hr) + ":" + ((sTm.tm_min < 10) ? "0" : "") + std::to_string(sTm.tm_min) + ((sTm.tm_hour < 12) ? "am" : "pm") + " - ";
                hr = eTm.tm_hour;
                str += Utils::Time::tmToString(eTm, d, m, y) + std::to_string((hr > 12) ? hr - 12 : hr) + ":" + ((eTm.tm_min < 10) ? "0" : "") + std::to_string(eTm.tm_min) + ((eTm.tm_hour < 12) ? "am" : "pm");
                ls->setTimeString(str);

                ls->setPlaytimeString(Utils::Time::playtimeToString(stats[i].playtime, " and "));
            }

            // Add percentage of total play time
            double percent = 100 * ((double)playtime / ((ps->playtime == 0) ? playtime : ps->playtime));
            percent = Utils::roundToDecimalPlace(percent, 2);
            if (percent < 0.01) {
                str = "< 0.01%";
            } else {
                str = Utils::truncateToDecimalPlace(std::to_string(percent), 2) + "%";
            }
            ls->setPercentageString(str);

            this->list->addElement(ls);
        }

        delete ps;
    }

    void Details::setupSessionHelp() {
        this->msgbox->emptyBody();
        this->msgbox->close(false);

        int bw, bh;
        this->msgbox->getBodySize(&bw, &bh);
        Aether::Element * body = new Aether::Element(0, 0, bw, bh);
        Aether::TextBlock * tb = new Aether::TextBlock(50, 40, "A Play Session represents the time between when a game was launched to when it was quit in succession.", 22, bw - 100);
        tb->setColour(this->app->theme()->text());
        body->addElement(tb);
        tb = new Aether::TextBlock(50, tb->y() + tb->h() + 20, "The time of launch, time of exit, play time and percentage of overall playtime is shown.\n\nSelect a session to view a more detailed breakdown.\n\nNote: Sessions with an asterisk start before and/or end after the selected date. Their playtime has been adjusted to reflect the current time range, however their breakdown will show the full duration of the session.", 20, bw - 100);
        tb->setColour(this->app->theme()->mutedText());
        body->addElement(tb);
        body->setWH(bw, tb->y() + tb->h() + 40);
        this->msgbox->setBodySize(body->w(), body->h());
        this->msgbox->setBody(body);

        this->app->addOverlay(this->msgbox);
    }

    void Details::setupSessionBreakdown(NX::PlaySession s) {
        this->panel->close(false);
        this->panel->setPlaytime(Utils::Time::playtimeToString(s.playtime, " and "));
        this->panel->setLength(Utils::Time::playtimeToString(s.endTimestamp - s.startTimestamp, " and "));

        // Container element to prevent stretching of text in list
        std::vector<NX::PlayEvent> events = this->app->playdata()->getPlayEvents(s.startTimestamp, s.endTimestamp, this->app->activeTitle()->titleID(), this->app->activeUser()->ID());
        struct tm lastTime = Utils::Time::getTm(0);
        time_t lastTs = 0;
        for (size_t i = 0; i < events.size(); i++) {
            // Print time of event in 12 hour format
            struct tm time = Utils::Time::getTm(events[i].clockTimestamp);
            bool isAM = ((time.tm_hour < 12) ? true : false);
            if (time.tm_hour == 0) {
                time.tm_hour = 12;
            }
            std::string str = std::to_string(((time.tm_hour > 12) ? time.tm_hour - 12 : time.tm_hour)) + ":" + ((time.tm_min < 10) ? "0" : "") + std::to_string(time.tm_min) + ((isAM) ? "am" : "pm") + " - ";

            // Add date string if new day
            if (Utils::Time::areDifferentDates(lastTime, time)) {
                struct tm now = Utils::Time::getTmForCurrentTime();
                Aether::Text * t = new Aether::Text(0, 10, Utils::Time::tmToString(time, true, true, (time.tm_year != now.tm_year)), 18);
                t->setColour(this->app->theme()->text());
                Aether::Element * c = new Aether::Element(0, 0, 100, t->h() + 20);
                c->addElement(t);
                this->panel->addListItem(c);
            }
            lastTime = time;

            // Concatenate event type onto time string
            bool addPlaytime = false;
            switch (events[i].eventType) {
                case NX::EventType::Applet_Launch:
                    str += "Application Launched";
                    lastTs = events[i].steadyTimestamp;
                    break;

                case NX::EventType::Applet_InFocus:
                    if (events[i-1].eventType == NX::EventType::Account_Active || events[i-1].eventType == NX::EventType::Applet_Launch) {
                        continue;
                    }
                    str += "Application Resumed";
                    lastTs = events[i].steadyTimestamp;
                    break;

                case NX::EventType::Applet_OutFocus:
                    if (events[i+1].eventType == NX::EventType::Account_Inactive || events[i+1].eventType == NX::EventType::Applet_Exit) {
                        continue;
                    }
                    str += "Application Suspended";
                    addPlaytime = true;
                    break;

                case NX::EventType::Applet_Exit:
                    str += "Application Closed";
                    addPlaytime = true;
                    break;

                default:
                    continue;
                    break;
            }

            // Create + add string element
            Aether::Text * t = new Aether::Text(0, 0, str, 18);
            t->setColour(this->app->theme()->mutedText());
            Aether::Element * c = new Aether::Element(0, 0, 100, t->h() + 5);
            c->addElement(t);

            // Add playtime from last "burst"
            if (addPlaytime) {
                t = new Aether::Text(t->x() + t->w() + 10, t->y() + t->h()/2, "(" + Utils::Time::playtimeToString(events[i].steadyTimestamp - lastTs, " and ") + ")", 16);
                t->setY(t->y() - t->h()/2);
                t->setColour(this->app->theme()->accent());
                c->addElement(t);
            }
            this->panel->addListItem(c);
        }

        this->app->addOverlay(this->panel);
    }

    void Details::onLoad() {
        // Render user's image
        this->userimage = new Aether::Image(1155, 14, this->app->activeUser()->imgPtr(), this->app->activeUser()->imgSize(), 4, 4);
        this->userimage->setWH(60, 60);
        this->addElement(this->userimage);

        // Render user's name
        this->username = new Aether::Text(1135, 45, this->app->activeUser()->username(), 20);
        this->username->setXY(this->username->x() - this->username->w(), this->username->y() - this->username->h()/2);
        this->username->setColour(this->app->theme()->mutedText());
        this->addElement(this->username);

        // Render title icon
        this->icon = new Aether::Image(65, 15, this->app->activeTitle()->imgPtr(), this->app->activeTitle()->imgSize(), 4, 4);
        this->icon->setWH(60, 60);
        this->addElement(this->icon);

        // Render title name and limit length
        this->title = new Aether::Text(145, 45, this->app->activeTitle()->name(), 28);
        this->title->setY(this->title->y() - this->title->h()/2);
        this->title->setColour(this->app->theme()->text());
        if (this->title->w() > this->username->x() - this->title->x() - 60) {
            this->title->setW(this->username->x() - this->title->x() - 60);
            this->title->setScroll(true);
        }
        this->addElement(this->title);

        // Create list (graph + play sessions)
        this->list = new Aether::List(40, 88, 840, 559);
        this->list->setScrollBarColour(this->app->theme()->mutedLine());
        this->addElement(this->list);

        // Add heading and L
        this->header = new Aether::Container(0, 0, 100, 70);
        Aether::Element * e = new Aether::Element(this->header->x(), this->header->y(), 80, 50);
        Aether::Text * t = new Aether::Text(e->x() + 10, e->y(), "\uE0E4", 20, Aether::FontType::Extended); // L
        t->setY(e->y() + (e->h() - t->h())/2);
        t->setColour(this->app->theme()->mutedText());
        e->addElement(t);
        t = new Aether::Text(e->x(), e->y(), "\uE149", 26, Aether::FontType::Extended); // <
        t->setXY(e->x() + e->w() - t->w() - 10, e->y() + (e->h() - t->h())/2);
        t->setColour(this->app->theme()->text());
        e->addElement(t);
        e->setCallback([this](){
            this->app->decreaseDate();
        });
        this->header->addElement(e);

        // Do this here as it adjusts header's width to the list's width
        this->list->addElement(this->header);

        // R button
        e = new Aether::Element(this->header->x() + this->header->w() - 80, this->header->y(), 80, 50);
        t = new Aether::Text(e->x() + 10, e->y(), "\uE14A", 26, Aether::FontType::Extended); // >
        t->setY(e->y() + (e->h() - t->h())/2);
        t->setColour(this->app->theme()->text());
        e->addElement(t);
        t = new Aether::Text(e->x(), e->y(), "\uE0E5", 20, Aether::FontType::Extended); // R
        t->setXY(e->x() + e->w() - t->w() - 10, e->y() + (e->h() - t->h())/2);
        t->setColour(this->app->theme()->mutedText());
        e->addElement(t);
        e->setCallback([this](){
            this->app->increaseDate();
        });
        this->header->addElement(e);

        // Add graph heading to container
        this->graphHeading = new Aether::Text(this->header->x(), this->header->y(), "", 22);
        this->graphHeading->setColour(this->app->theme()->text());
        this->header->addElement(this->graphHeading);
        this->graphSubheading = new Aether::Text(this->header->x(), this->graphHeading->y() + 30, "", 16);
        this->graphSubheading->setColour(this->app->theme()->mutedText());
        this->header->addElement(this->graphSubheading);

        // Setup graph
        this->graph = new CustomElm::Graph(0, 0, this->list->w(), 350, 1);
        this->graph->setBarColour(this->app->theme()->accent());
        this->graph->setLabelColour(this->app->theme()->text());
        this->graph->setLineColour(this->app->theme()->mutedLine());
        this->graph->setValueVisibility(this->app->config()->gGraph());
        this->list->addElement(this->graph);
        this->list->addElement(new Aether::ListSeparator(30));

        e = new Aether::Element(0, 0, 100, 30);
        this->graphTotal = new Aether::Text(e->x(), e->y(), "Total Play Time: ", 20);
        this->graphTotal->setColour(this->app->theme()->text());
        e->addElement(this->graphTotal);
        this->graphTotalSub = new Aether::Text(this->graphTotal->x() + this->graphTotal->w(), e->y(), "", 20);
        this->graphTotalSub->setColour(this->app->theme()->accent());
        e->addElement(this->graphTotalSub);
        this->list->addElement(e);
        this->list->addElement(new Aether::ListSeparator(20));

        // Add play sessions heading
        Aether::ListHeadingHelp * lhh = new Aether::ListHeadingHelp("Play Sessions", [this](){
            this->setupSessionHelp();
        });
        lhh->setHelpColour(this->app->theme()->mutedText());
        lhh->setRectColour(this->app->theme()->mutedLine());
        lhh->setTextColour(this->app->theme()->text());
        this->list->addElement(lhh);

        // Keep pointer to this element to allow removal
        this->topElm = new Aether::ListSeparator(20);
        this->list->addElement(this->topElm);

        // Get play sessions
        this->updateGraph();
        this->updateSessions();
        this->setFocussed(this->list);

        // Add side stats
        NX::PlayStatistics * ps = this->app->playdata()->getStatisticsForUser(this->app->activeTitle()->titleID(), this->app->activeUser()->ID());
        this->playtime = new Aether::Text(1070, 220, Utils::Time::playtimeToString(ps->playtime, " and "), 20);
        this->playtime->setColour(this->app->theme()->accent());
        this->playtime->setX(this->playtime->x() - this->playtime->w()/2);
        this->addElement(this->playtime);

        this->avgplaytime = new Aether::Text(1070, 310, Utils::Time::playtimeToString(ps->playtime / ps->launches, ", "), 20);
        this->avgplaytime->setColour(this->app->theme()->accent());
        this->avgplaytime->setX(this->avgplaytime->x() - this->avgplaytime->w()/2);
        this->addElement(this->avgplaytime);

        this->timeplayed = new Aether::Text(1070, 400, Utils::formatNumberComma(ps->launches) + " times", 20);
        this->timeplayed->setColour(this->app->theme()->accent());
        this->timeplayed->setX(this->timeplayed->x() - this->timeplayed->w()/2);
        this->addElement(this->timeplayed);

        this->firstplayed = new Aether::Text(1070, 490, Utils::Time::timestampToString(pdmPlayTimestampToPosix(ps->firstPlayed)), 20);
        this->firstplayed->setColour(this->app->theme()->accent());
        this->firstplayed->setX(this->firstplayed->x() - this->firstplayed->w()/2);
        this->addElement(this->firstplayed);

        this->lastplayed = new Aether::Text(1070, 580, Utils::Time::timestampToString(pdmPlayTimestampToPosix(ps->lastPlayed)), 20);
        this->lastplayed->setColour(this->app->theme()->accent());
        this->lastplayed->setX(this->lastplayed->x() - this->lastplayed->w()/2);
        this->addElement(this->lastplayed);
        delete ps;

        // Create blank messagebox
        this->msgbox = new Aether::MessageBox();
        this->msgbox->addTopButton("Close", [this](){
            this->msgbox->close(true);
        });
        this->msgbox->setLineColour(this->app->theme()->mutedLine());
        this->msgbox->setRectangleColour(this->app->theme()->altBG());
        this->msgbox->setTextColour(this->app->theme()->accent());

        // Create blank panel
        this->panel = new CustomOvl::PlaySession();
        this->panel->setAccentColour(this->app->theme()->accent());
        this->panel->setBackgroundColour(this->app->theme()->altBG());
        this->panel->setLineColour(this->app->theme()->fg());
        this->panel->setMutedLineColour(this->app->theme()->mutedLine());
        this->panel->setTextColour(this->app->theme()->text());
    }

    void Details::onUnload() {
        this->removeElement(this->playtime);
        this->removeElement(this->avgplaytime);
        this->removeElement(this->timeplayed);
        this->removeElement(this->firstplayed);
        this->removeElement(this->lastplayed);
        this->removeElement(this->icon);
        this->removeElement(this->list);
        this->removeElement(this->title);
        this->removeElement(this->userimage);
        this->removeElement(this->username);
        delete this->msgbox;
        delete this->panel;
    }
};