#include <iostream>
#include <math.h>

#include "MyStrategy.h"

using namespace std;

MyStrategy::MyStrategy() {
    _first = true;
}

void MyStrategy::move(const Hockeyist& self, const World& world, const Game& game, Move& move) {
    _game = GameF(game);

    if (_first) {
        _fix = CoordFix(_game.getWorldWidth(), _game.getWorldHeight(),
                        world.getMyPlayer().getNetBack() < _game.getWorldWidth() / 2);
    }

    _fix.setInvY(false);

    _self = HockeyistF(self, &_fix);
    _world = WorldF(world, &_fix, _game);
    _move = MoveF(&move, &_fix);

    if (_first) {
        _first = false;

        _attackAreaDestX = AttackAreaX0 + self.getRadius();
        _attackAreaDestY = AttackAreaY0 + _game.getPuckBindingRange() + _world.getPuck().getRadius();

        _attackDestX = _world.getOpponentPlayer().getNetFront();
        _attackDestY0 = _world.getOpponentPlayer().getNetTop() + _world.getPuck().getRadius();
        _attackDestY1 = _fix.invcY(_attackDestY0);
    }

    act();
}

//

double MyStrategy::conv0(double x, double z) {
    return (x * M_PI / z / 2);
}

double MyStrategy::conv1(double x, double z) {
    return M_PI / 2 * (x + M_PI - 2 * z) / (M_PI - z);
}

void MyStrategy::goAngle(double a) {
    const double z = M_PI * 2 / 3;
    const double m = 6;
    const double n = -4;
    const double alpha = M_PI / 18;
    const double beta = M_PI - alpha;

    _move.setTurn(a);

    a = fabs(a);

    double realspeed = Pif(_self.getSpeedX(), _self.getSpeedY());
    _move.setSpeedUp(0);

    if (a <= z) {
        double maxspeed = ctg(conv0(a, z)) * m / ctg(conv0(alpha, z));
        if (maxspeed > realspeed ) {
            _move.setSpeedUp(1);
        }
    } else {
        double minspeed = ctg(conv1(a, z)) * n / ctg(conv1(beta, z));
        if (minspeed < n) {
            _move.setSpeedUp(-1);
        }
    }
}

void MyStrategy::gotoXY(double x, double y) {
    goAngle(_self.getAngleTo(x, y));
}

void MyStrategy::gotoXY(const UnitF& u) {
    gotoXY(u.getX(), u.getY());
}

//

bool MyStrategy::isInAttackArea(double x, double y) {
    return (x > AttackAreaX0 && x < AttackAreaX1 && y > AttackAreaY0 && y < AttackAreaY1);
}

bool MyStrategy::isInAttackArea(const UnitF& u) {
    return isInAttackArea(u.getX(), u.getY());
}

//

bool MyStrategy::isNearStick(double x, double y) {
    return ( _self.getDistanceTo(x, y) <= _game.getStickLength() ) &&
           ( fabs(_self.getAngleTo(x, y)) <= _game.getStickSector() / 2 );
}

bool MyStrategy::isNearStick(const UnitF& u) {
    return isNearStick(u.getX(), u.getY());
}

//

void MyStrategy::act() {
//    if (_self.getTeammateIndex() == 1) {
//        gotoXY(0, 0);
//        return;
//    }

    _fix.setInvY(_self.getY() > _game.getWorldHeight() / 2);

    if (_world.getPuck().getOwnerHockeyistId() == _self.getId()) {
        if (isInAttackArea(_self)) {
            double angle = _self.getAngleTo(_attackDestX, _attackDestY1) - StrikeAngleCorrection;
            goAngle(angle - StrikeAnglePrecision / 2);

            if ((angle > 0) && (angle < StrikeAnglePrecision)) {
                _move.setAction(STRIKE);
            }
        } else {
            gotoXY(_attackAreaDestX, _attackAreaDestY);
        }
    } else {
        if (_world.getPuck().getOwnerPlayerId() == _self.getPlayerId()) {
            if (DefenceArea.isInU(_self)) {
                _move.setTurn(_self.getAngleTo(_world.getPuck()));
            } else {
                gotoXY(DefenceArea.getX(), DefenceArea.getY());
            }
        } else {
            gotoXY(_world.getPuck());
            _move.setAction(TAKE_PUCK);
        }

        if (!isNearStick(_world.getPuck())) {
            bool teammateNear = false;
            bool opponentNear = false;

            const vector<HockeyistF> hockeyists = _world.getHockeyists();
            for (vector<HockeyistF>::const_iterator it = hockeyists.cbegin(); it != hockeyists.cend(); ++it) {
                const HockeyistF& h = *it;
                if ((h.getRemainingKnockdownTicks() == 0) && (h.getType() != GOALIE) && isNearStick(h)) {
                    if (h.isTeammate()) {
                        teammateNear = true;
                    } else {
                        opponentNear = true;
                    }
                }
            }

            if (opponentNear && !teammateNear) {
                _move.setAction(STRIKE);
            }
        }
    }
}
