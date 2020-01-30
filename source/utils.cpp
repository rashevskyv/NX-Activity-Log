#include <fstream>
#include "Utils.hpp"

// Maximum number of titles to read using pdm
#define MAX_TITLES 2000

// Comparison of AccountUids
bool operator == (const AccountUid &a, const AccountUid &b) {
    if (a.uid[0] == b.uid[0] && a.uid[1] == b.uid[1]) {
        return true;
    }
    return false;
}

namespace Utils {
    void copyFile(std::string src, std::string dest) {
        std::ifstream srcF(src, std::ios::binary);
        std::ofstream destF(dest, std::ios::binary);

        destF << srcF.rdbuf();

        srcF.close();
        destF.flush();
        destF.close();
    }

    // Add commas to a number (only does one but shhh)
    std::string formatNumberComma(u32 number) {
        std::string s = std::to_string(number);
        if (s.length() > 3) {
            return s.substr(0, s.length() - 3) + "," + s.substr(s.length() - 3, 3);
        }
        return s;
    }

    ThemeType getHorizonTheme() {
        ColorSetId thm;
        Result rc = setsysGetColorSetId(&thm);
        if (R_SUCCEEDED(rc)) {
            switch (thm) {
                case ColorSetId_Light:
                    return ThemeType::T_Light;
                    break;

                case ColorSetId_Dark:
                    return ThemeType::T_Dark;
                    break;
            }
        }

        // If it fails return dark
        return ThemeType::T_Dark;
    }

    std::vector<User *> getUserObjects() {
        // Get IDs
        std::vector<User *> users;
        AccountUid userIDs[ACC_USER_LIST_SIZE];
        s32 num = 0;
        Result rc = accountListAllUsers(userIDs, ACC_USER_LIST_SIZE, &num);

        if (R_SUCCEEDED(rc)) {
            // Create objects and insert into vector
            for (s32 i = 0; i < num; i++) {
                users.push_back(new User(userIDs[i]));
            }
        }

        // Returns an empty vector if an error occurred
        return users;
    }

    std::vector<Title *> getTitleObjects(std::vector<User *> u) {
        Result rc;

        // Get ALL played titles for ALL users
        // (this doesn't include installed games that haven't been played)
        std::vector<u64> playedIDs;
        for (unsigned short i = 0; i < u.size(); i++) {
            s32 playedTotal = 0;
            u64 * tmpIDs = new u64[MAX_TITLES];
            pdmqryQueryRecentlyPlayedApplication(u[i]->ID(), tmpIDs, MAX_TITLES, &playedTotal);

            // Push back ID if not already in the vector
            for (s32 j = 0; j < playedTotal; j++) {
                bool found = false;
                for (size_t k = 0; k < playedIDs.size(); k++) {
                    if (playedIDs[k] == tmpIDs[j]) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    playedIDs.push_back(tmpIDs[j]);
                }
            }
            delete[] tmpIDs;
        }

        // Get IDs of all installed titles
        std::vector<u64> installedIDs;
        NsApplicationRecord * records = new NsApplicationRecord[MAX_TITLES];
        s32 count = 0;
        s32 installedTotal = 0;
        while (true){
            rc = nsListApplicationRecord(records, MAX_TITLES, count, &installedTotal);
            // Break if at the end or no titles
            if (R_FAILED(rc) || installedTotal == 0){
                break;
            }
            count++;
            installedIDs.push_back(records->application_id);
        }
        delete[] records;

        // Create Title objects from IDs
        std::vector<Title *> titles;
        for (size_t i = 0; i < playedIDs.size(); i++) {
            // Loop over installed titles to determine if installed or not
            bool installed = false;
            for (size_t j = 0; j < installedIDs.size(); j++) {
                if (installedIDs[j] == playedIDs[i]) {
                    installed = true;
                    break;
                }
            }

            titles.push_back(new Title(playedIDs[i], installed));
        }

        return titles;
    }

    void startServices() {
        accountInitialize(AccountServiceType_System);
        nsInitialize();
        pdmqryInitialize();
        romfsInit();
        setsysInitialize();
    }

    void stopServices() {
        accountExit();
        nsExit();
        pdmqryExit();
        romfsExit();
        setsysExit();
    }
};