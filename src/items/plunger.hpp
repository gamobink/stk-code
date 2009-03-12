//  $Id: missile.hpp 1284 2007-11-08 12:31:54Z hikerstk $
//
//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2007 Joerg Henrichs
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef HEADER_MISSILE_HPP
#define HEADER_MISSILE_HPP

#ifdef HAVE_IRRLICHT
#include "irrlicht.h"
using namespace irr;
#endif
#include "flyable.hpp"

class RubberBand;
class Kart;
class MovingPhysics;

class Plunger : public Flyable
{
private:
    /** The rubber band attached to a plunger. */
    RubberBand  *m_rubber_band;
    /** Timer to keep the plunger alive while the rubber band is working. */
    float        m_keep_alive;

    bool m_reverse_mode;
public:
                 Plunger(Kart *kart);
                ~Plunger();
#ifdef HAVE_IRRLICHT
                static  void init(const lisp::Lisp* lisp, scene::IMesh* missile);
#else
    static  void init     (const lisp::Lisp* lisp, ssgEntity* missile);
#endif
    /** Sets the keep-alive value. Setting it to 0 will remove the plunger
     *  at the next update - which is used if the rubber band snaps. 
     */
    void         setKeepAlive(float t) {m_keep_alive = t;}
    virtual void update   (float dt);
    virtual void hitTrack ();
    virtual void hit      (Kart *kart, MovingPhysics *mp=NULL);

    /** A plunger does not explode if it is removed. */
    virtual bool needsExplosion() const {return false;}
};   // Plunger

#endif
