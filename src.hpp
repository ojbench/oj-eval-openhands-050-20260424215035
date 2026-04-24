#pragma once
#include <exception>
#include <optional>

class InvalidOperation : public std::exception {
public:
    const char* what() const noexcept override { return "invalid operation"; }
};

struct PlayInfo {
    int dummyCount = 0;
    int magnifierCount = 0;
    int converterCount = 0;
    int cageCount = 0;
};

class GameState {
public:
    enum class BulletType { Live, Blank };
    enum class ItemType { Dummy, Magnifier, Converter, Cage };

    GameState() : currentPlayer_(0) {
        hp_[0] = hp_[1] = 5;
        liveCount_ = blankCount_ = 0;
        cagePending_[0] = cagePending_[1] = false;
        cageUsedThisTurn_[0] = cageUsedThisTurn_[1] = false;
        lockedNext_.reset();
    }

    void fireAtOpponent(BulletType topBulletBeforeAction) {
        consumeBullet(topBulletBeforeAction);
        const int opponent = currentPlayer_ ^ 1;
        if (topBulletBeforeAction == BulletType::Live) {
            --hp_[opponent];
            if (hp_[opponent] <= 0) return; // game ends immediately
        }
        endTurnDueToShot(true);
    }

    void fireAtSelf(BulletType topBulletBeforeAction) {
        consumeBullet(topBulletBeforeAction);
        const bool isLive = (topBulletBeforeAction == BulletType::Live);
        if (isLive) {
            --hp_[currentPlayer_];
            if (hp_[currentPlayer_] <= 0) return; // game ends immediately
        }
        endTurnDueToShot(isLive);
    }

    void useDummy(BulletType topBulletBeforeUse) {
        if (players_[currentPlayer_].dummyCount <= 0) throw InvalidOperation();
        --players_[currentPlayer_].dummyCount;
        consumeBullet(topBulletBeforeUse);
        // Does not end turn
    }

    void useMagnifier(BulletType topBulletBeforeUse) {
        if (players_[currentPlayer_].magnifierCount <= 0) throw InvalidOperation();
        --players_[currentPlayer_].magnifierCount;
        lockedNext_ = topBulletBeforeUse;
        // Does not consume bullet
    }

    void useConverter(BulletType topBulletBeforeUse) {
        if (players_[currentPlayer_].converterCount <= 0) throw InvalidOperation();
        --players_[currentPlayer_].converterCount;
        // Reveal and flip next bullet, and adjust counts accordingly
        if (topBulletBeforeUse == BulletType::Live) {
            // flip live -> blank
            --liveCount_;
            ++blankCount_;
            lockedNext_ = BulletType::Blank;
        } else {
            // flip blank -> live
            --blankCount_;
            ++liveCount_;
            lockedNext_ = BulletType::Live;
        }
        // Does not consume bullet
    }

    void useCage() {
        if (players_[currentPlayer_].cageCount <= 0) throw InvalidOperation();
        if (cageUsedThisTurn_[currentPlayer_]) throw InvalidOperation();
        --players_[currentPlayer_].cageCount;
        cagePending_[currentPlayer_] = true;
        cageUsedThisTurn_[currentPlayer_] = true;
    }

    void reloadBullets(int liveCount, int blankCount) {
        liveCount_ = liveCount;
        blankCount_ = blankCount;
        lockedNext_.reset();
    }

    void reloadItem(int playerId, ItemType item) {
        PlayInfo &p = players_[playerId];
        switch (item) {
            case ItemType::Dummy: ++p.dummyCount; break;
            case ItemType::Magnifier: ++p.magnifierCount; break;
            case ItemType::Converter: ++p.converterCount; break;
            case ItemType::Cage: ++p.cageCount; break;
        }
    }

    double nextLiveBulletProbability() const {
        if (lockedNext_.has_value()) {
            return (*lockedNext_ == BulletType::Live) ? 1.0 : 0.0;
        }
        const int total = liveCount_ + blankCount_;
        if (total <= 0) return 0.0;
        return static_cast<double>(liveCount_) / static_cast<double>(total);
    }

    double nextBlankBulletProbability() const {
        if (lockedNext_.has_value()) {
            return (*lockedNext_ == BulletType::Blank) ? 1.0 : 0.0;
        }
        const int total = liveCount_ + blankCount_;
        if (total <= 0) return 0.0;
        return static_cast<double>(blankCount_) / static_cast<double>(total);
    }

    int winnerId() const {
        if (hp_[0] <= 0) return 1;
        if (hp_[1] <= 0) return 0;
        return -1;
    }

private:
    void consumeBullet(BulletType b) {
        if (b == BulletType::Live) {
            --liveCount_;
        } else {
            --blankCount_;
        }
        lockedNext_.reset();
    }

    void endTurnDueToShot(bool wouldEnd) {
        if (!wouldEnd) return; // continue same player's turn
        if (cagePending_[currentPlayer_]) {
            // Cancel end of turn once
            cagePending_[currentPlayer_] = false;
            return; // stay on same player, still same turn (usedThisTurn remains true)
        }
        // Switch turn
        currentPlayer_ ^= 1;
        // Reset per-turn cage usage flag for the new active player
        cageUsedThisTurn_[currentPlayer_] = false;
    }

    int hp_[2];
    int currentPlayer_;
    int liveCount_ = 0;
    int blankCount_ = 0;
    PlayInfo players_[2];
    bool cagePending_[2];
    bool cageUsedThisTurn_[2];
    std::optional<BulletType> lockedNext_;
};
