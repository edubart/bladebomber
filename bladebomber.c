#include <riv.h>
#include "utils.h"

// #define DEBUG_BBOX
// #define DEBUG_SPRS

//------------------------------------------------------------------------------
// Constants

enum {
  SCREEN_PIXELS = 256,
  TILE_PIXELS = 16,
  MAP_SIZE = 64,
  NUM_LEVELS = 4,
  MAX_OBJECTS = 4096,
  SPRITESHEET_COLUMNS = 16,
  SPRITESHEET_GAME = 1,
};

typedef enum MAP_LAYERS {
  MAP_LAYER_GROUND = 0,
  MAP_LAYER_BOTTOM_ITEMS,
  MAP_LAYER_CREATURES,
  MAP_LAYER_WALLS,
  MAP_LAYER_TOP_ITEMS,
  MAP_LAYER_EFFECTS,
  NUM_MAP_LAYERS = 5,
} MAP_LAYERS;

typedef enum TYPE_ID {
  TYPE_NONE = 0,
  TYPE_GROUND = (1 << 0),
  TYPE_ITEM = (1 << 1),
  TYPE_CREATURE = (1 << 2),
  TYPE_WALL = (1 << 3),
  TYPE_MONSTER = (1 << 4) | TYPE_CREATURE,
  TYPE_PLAYER = (1 << 5) | TYPE_CREATURE,
  TYPE_EFFECT = (1 << 6),
  NUM_TYPES,
} TYPE_ID;

typedef enum GFX_ID {
  GFX_GROUND_SPIKES = 118,
  GFX_GROUND_SPIKES_HURT = 123,
  GFX_GROUND_STAIRS = 49,
  GFX_ITEM_HEART = 58,
  GFX_ITEM_CLOCK = 41,
  GFX_ITEM_POTION = 7,
  GFX_ITEM_COIN = 1,
  GFX_ITEM_TORCH = 150,
  GFX_ITEM_CHEST = 86,
  GFX_ITEM_CHEST_OPEN = 94,
  GFX_ITEM_KEY = 5,
  GFX_ITEM_BOMB = 103,
  GFX_ITEM_UPGRADE_BOMB = 102,
  GFX_ITEM_UPGRADE_BLADE = 57,
  GFX_WALL_CLOSED_DOOR = 196,
  GFX_WALL_AUTO_DOOR = 202,
  GFX_WALL_ARCH = 61,
  GFX_WALL_DOOR_OPEN_TLR = 194,
  GFX_WALL_DOOR_OPEN_BL = 210,
  GFX_WALL_DOOR_OPEN_BR = 211,
  GFX_MONSTER_BAT = 70,
  GFX_MONSTER_SLIME = 96,
  GFX_MONSTER_SLIME_BOSS = 112,
  GFX_MONSTER_GOBLIN = 64,
  GFX_PLAYER = 128,
  GFX_PLAYER_IDLE = GFX_PLAYER,
  GFX_PLAYER_WALK = 144,
  GFX_EFFECT_ATTACK = 156,
  GFX_EFFECT_DUST = 134,
  GFX_EFFECT_EXPLOSION = 160,
  NUM_GFX = 256
} GFX_ID;

typedef enum SFX_ID {
  SFX_COIN_PICKUP,
  SFX_COIN_DROP,
  SFX_POTION_PICKUP,
  SFX_DASH,
  SFX_ATTACK,
  SFX_HURT_MONSTER,
  SFX_HURT_PLAYER,
  SFX_DIE_MONSTER,
  SFX_DIE_PLAYER,
  SFX_OPEN_DOOR,
  SFX_KEY_PICKUP,
  SFX_CHEST_OPEN,
  SFX_EXPLOSION,
  SFX_UPGRADE,
  SFX_GAME_COMPLETE1,
  SFX_GAME_COMPLETE2,
  SFX_GAME_COMPLETE3,
  NUM_SFX,
  NUM_SFX_CHANNELS = 4
} SFX_ID;

//------------------------------------------------------------------------------
// Structures

typedef struct Thing {
  u8 type; // type id
  u16 spr; // sprite id
  u8 layer; // map layer
  u16 id; // object id
  bool removed;
  bool phantom;
  u64 spawn_frame;
  vec2 pos; // top left coordinates for the sprite
  vec2 spawn_pos;
  recti bbox;
  recti spr_bbox; // bounding box inside the sprite
  vec2i spr_tiles; // number of tiles of this sprite
  vec2i spr_scale; // sprite scale multiplier
  u16 spr_frame_duration;
  u16 spr_frames;
  u16 spr_loop_delay;
} Thing;

typedef struct Item {
  Thing thing;
  u64 trigger_frame;
  i64 damage;
} Item;

typedef struct Creature {
  Thing thing;
  f64 speed;
  f64 slowdown;
  i64 health;
  u16 attack1_delay;
  u16 attack2_delay;
  u16 attack1_damage;
  u16 attack2_damage;
  u64 slowdown_until_frame;
  u64 attack1_frame;
  u64 attack2_frame;
  u64 hurt_frame;
  u64 die_frame;
  bool moving;
} Creature;

typedef struct Monster {
  union {
    Thing thing;
    Creature creature;
  };
  f64 sight;
} Monster;

typedef struct Player {
  union {
    Thing thing;
    Creature creature;
  };
  f64 dash_power;
  u64 dash_frame;
  i16 dash_delay;
  i16 dash_duration;
  u16 last_step_spr;
} Player;

typedef struct Effect {
  Thing thing;
} Effect;

typedef union Object {
  Thing thing;
  Item item;
  Creature creature;
  Monster monster;
  Player player;
  Effect effect;
} Object;

//------------------------------------------------------------------------------
// Data

// Graphics
Object gfx_objects[NUM_GFX] =
#include "gfx.h"
// Sounds
riv_waveform_desc sfx_descs[NUM_SFX][NUM_SFX_CHANNELS] =
#include "sfx.h"
// Maps
unsigned char maps[NUM_LEVELS][NUM_MAP_LAYERS][MAP_SIZE][MAP_SIZE] =
#include "maps.h"

//------------------------------------------------------------------------------
// Game state

Object objects[MAX_OBJECTS];
u32 object_count;
Player* main_player;
i64 picked_keys;
i64 level;
Object *first_collidable;
i64 next_level;
u64 shake_frame;
u64 end_frame;
i64 kills;
i64 coins;

//------------------------------------------------------------------------------
// Game utils

Object *spawn(u16 gfx, u16 l, f64 x, f64 y);

void sfx(u16 sfx) {
  for (u64 i=0;i<NUM_SFX_CHANNELS && sfx_descs[sfx][i].type != RIV_WAVEFORM_NONE;++i) {
    riv_waveform(&sfx_descs[sfx][i]);
  }
}

i64 timer_countdown(u64 start_frame, u64 duration) {
  return start_frame > 0 ? (duration - mini(riv->frame - start_frame, duration)) : 0;
}

Object *find_object_by_spr(u16 spr) {
  for (u32 i=1;i<=object_count;++i) {
    Object *object = &objects[i];
    if (object->thing.spr == spr) {
      return object;
    }
  }
  return NULL;
}

void end_game() {
  // game completed
  next_level = NUM_LEVELS;
  sfx(SFX_GAME_COMPLETE1);
  sfx(SFX_GAME_COMPLETE2);
  sfx(SFX_GAME_COMPLETE3);
  riv->quit_frame = riv->frame + 240; // quit in 4 seconds
  end_frame = riv->frame;
}

//------------------------------------------------------------------------------
// Thing

recti thing_bbox_at(Thing* thing, vec2 pos) {
  recti bbox = {
    thing->spr_bbox.x,
    thing->spr_bbox.y,
    thing->spr_bbox.width,
    thing->spr_bbox.height,
  };
  if (thing->spr_scale.x < 0) {
    bbox.x = (thing->spr_tiles.x * TILE_PIXELS) - (bbox.x + bbox.width);
  }
  if (thing->spr_scale.y < 0) {
    bbox.y = (thing->spr_tiles.y * TILE_PIXELS) - (bbox.y + bbox.height);
  }
  i64 sx = absi(thing->spr_scale.x);
  i64 sy = absi(thing->spr_scale.x);
  bbox.x *= sx;
  bbox.width *= sx;
  bbox.y *= sy;
  bbox.height *= sy;
  bbox.x += ifloor(pos.x);
  bbox.y += ifloor(pos.y);
  return bbox;
}

u32 thing_get_spr_anim(Thing* thing, u32 spr) {
  if (spr == 0) {
    spr = thing->spr;
  }
  if (thing->spr_frame_duration > 0) {
    i64 frame = (riv->frame - thing->spawn_frame) / thing->spr_frame_duration;
    if (thing->spr_loop_delay > 0) {
      frame = maxi(frame % (thing->spr_frames + thing->spr_loop_delay) - thing->spr_loop_delay, 0);
    }
    u32 anim_spr = spr + (frame % thing->spr_frames) * thing->spr_tiles.x;
    if (thing->spr_tiles.y > 1) {
      spr = anim_spr + ((anim_spr / SPRITESHEET_COLUMNS) - (spr / SPRITESHEET_COLUMNS)) *
                       (thing->spr_tiles.y - 1) * SPRITESHEET_COLUMNS;
    } else {
      spr = anim_spr;
    }
  }
  return spr;
}

void thing_draw(Thing* thing, u32 spr, i64 ox, i64 oy) {
  if (thing->spr == 0) {
    return;
  }
  // disable blending for opaque ground sprites (optimization)
  bool opaque = thing->layer == 0;
  if (opaque) {
    riv->draw.color_key_disabled = true;
  }
  // draw sprite
  spr = thing_get_spr_anim(thing, spr);
  vec2i pos = ifloor_vec2(thing->pos);
  riv_draw_sprite(spr, SPRITESHEET_GAME, pos.x + ox, pos.y + oy, thing->spr_tiles.x, thing->spr_tiles.y, thing->spr_scale.x, thing->spr_scale.y);
  // restore blending
  if (opaque) {
    riv->draw.color_key_disabled = false;
  }
#ifdef DEBUG_BBOX
  // draw debug bbox
  recti bbox = thing->bbox;
  if (thing->layer > 0) {
    riv_draw_rect_line(bbox.x, bbox.y, bbox.width, bbox.height, RIV_COLOR_LIGHTGREEN);
  }
#ifdef DEBUG_SPRS
  riv_draw_text(riv_tprintf("%d", spr), RIV_SPRITESHEET_FONT_3X5, RIV_CENTER,
    bbox.x + bbox.width / 2, bbox.y + bbox.height / 2, 1, RIV_COLOR_WHITE);
#endif
#endif
}

Object* thing_collides_with(Thing* thing, recti bbox, u8 type, Object* last) {
  for (u32 i=last ? last->thing.id+1 : 1;i<=object_count;++i) {
    Object *other = &objects[i];
    if ((other->thing.type & type) != 0 &&
        overlaps_recti(bbox, other->thing.bbox) &&
        &other->thing != thing && !other->thing.removed && !other->thing.phantom) {
      return other;
    }
  }
  return NULL;
}

bool thing_collides_at(Thing* thing, vec2 pos) {
  return thing_collides_with(thing, thing_bbox_at(thing, pos), TYPE_WALL | TYPE_CREATURE, first_collidable) != NULL;
}

bool thing_collides_with_player(Thing* thing) {
  return !main_player->thing.removed && overlaps_recti(thing->bbox, main_player->thing.bbox);
}

//------------------------------------------------------------------------------
// Creature

void creature_move(Creature* creature, vec2 delta) {
  if (delta.x != 0) { // move x
    f64 val = abst(delta.x);
    f64 sgn = sign(delta.x);
    vec2 start_pos = creature->thing.pos;
    vec2 pos = start_pos;
    // move pixel by pixel until it collides
    for (i64 d = 0; d <= iceil(val); ++d) {
      pos.x = start_pos.x+min(d, val)*sgn;
      if (thing_collides_at(&creature->thing, pos)) {
        break;
      }
      creature->thing.pos.x = pos.x;
      creature->thing.bbox = thing_bbox_at(&creature->thing, pos);
    }
  }
  if (delta.y != 0) { // move y
    f64 val = abst(delta.y);
    f64 sgn = sign(delta.y);
    vec2 start_pos = creature->thing.pos;
    vec2 pos = start_pos;
    // move pixel by pixel until it collides
    for (i64 d = 0; d <= iceil(val); ++d) {
      pos.y = start_pos.y+min(d, val)*sgn;
      if (thing_collides_at(&creature->thing, pos)) {
        break;
      }
      creature->thing.pos.y = pos.y;
      creature->thing.bbox = thing_bbox_at(&creature->thing, pos);
    }
  }
}

void creature_hit(Creature *creature, i64 damage) {
  if (creature->health == 0) {
    return;
  }
  creature->health = maxi(creature->health - damage, 0);
  creature->hurt_frame = riv->frame;
  sfx(creature == &main_player->creature ? SFX_HURT_PLAYER : SFX_HURT_MONSTER);
  if (creature->health == 0) { // died
    spawn(GFX_EFFECT_DUST, MAP_LAYER_EFFECTS, creature->thing.pos.x, creature->thing.pos.y);
    creature->die_frame = riv->frame + 8;
    if (creature == &main_player->creature) {
      sfx(SFX_DIE_PLAYER);
      riv->quit_frame = riv->frame + 180; // quit in 3 seconds
    } else {
      kills++;
      sfx(SFX_DIE_MONSTER);
    }
  }
}

void creature_pull_hit(Creature *creature, i64 xdir, i64 damage) {
  creature_hit(creature, damage);

  // pull back
  creature_move(creature, (vec2){xdir*4, 0});

  // slow it
  creature->slowdown = 0.5;
  creature->slowdown_until_frame = riv->frame + 30;
}

void creature_draw(Creature* creature, u32 spr, i64 ox, i64 oy) {
  if (spr == 0) {
    spr = creature->thing.spr;
  }
  if (creature->moving) {
    spr += SPRITESHEET_COLUMNS;
  }
  i64 hurt_ticks = riv->frame - creature->hurt_frame;
  if (hurt_ticks < 12 && creature->hurt_frame > 0) {
    riv->draw.pal_enabled = true;
    riv->draw.pal[0] = RIV_COLOR_RED; // red
    thing_draw(&creature->thing, spr, ox, oy-hurt_ticks/6);
    riv->draw.pal[0] = 0;
    riv->draw.pal_enabled = false;
  } else {
    thing_draw(&creature->thing, spr, ox, oy);
  }
}

void creature_update(Creature *creature) {
  if (creature->die_frame > 0 && riv->frame >= creature->die_frame) {
    creature->thing.removed = true;
  }
}

//------------------------------------------------------------------------------
// Monster

void monster_draw(Monster *monster) {
  i64 ox = 0;

  // push towards attack direction
  if (monster->creature.attack1_frame > 0) {
    i64 attack1_frame = timer_countdown(monster->creature.attack1_frame, monster->creature.attack1_delay);
    if (attack1_frame > 0) {
      ox = attack1_frame/8 * monster->thing.spr_scale.x;
    }
  }

  creature_draw(&monster->creature, 0, ox, 0);
}

void monster_update(Monster *monster) {
  creature_update(&monster->creature);

  vec2 delta = sub_vec2(main_player->thing.pos, monster->thing.pos);
  vec2 delta_sqr = sqr_vec2(delta);
  f64 dist_sqr = delta_sqr.x + delta_sqr.y;

  // attack if player is very near
  if (dist_sqr <= sqr(TILE_PIXELS) && riv->frame >= monster->creature.attack1_frame + monster->creature.attack1_delay &&
      riv->frame != monster->thing.spawn_frame) {
    monster->creature.attack1_frame = riv->frame;
    creature_hit(&main_player->creature, monster->creature.attack1_damage);
  }

  if (dist_sqr <= sqr(TILE_PIXELS*monster->sight)) { // player is near
    if (dist_sqr >= 1) {
      // move towards player
      f64 speed = monster->creature.speed;
      if (monster->creature.slowdown_until_frame > riv->frame) {
        speed *= monster->creature.slowdown;
      }
      vec2 move_delta = mul_vec2_scalar(delta, speed/sqrt(dist_sqr));
      creature_move(&monster->creature, move_delta);
      // turn towards player
      monster->thing.spr_scale.x = isign(move_delta.x) * absi(monster->thing.spr_scale.x);
    }
  } else if (dist_sqr <= sqr(TILE_PIXELS*12)) { // player is in sight
    // do random move
    vec2 pos = monster->thing.pos;
    pos.x += riv_rand_float()*2-1;
    pos.y += riv_rand_float()*2-1;
    if (!thing_collides_at(&monster->thing, pos)) {
      monster->thing.pos = pos;
      monster->thing.bbox = thing_bbox_at(&monster->thing, pos);
    }
  } else {
    // out of sight
  }
}

//------------------------------------------------------------------------------
// Slime Boss

void slime_boss_spawn_minion(Monster* monster, i64 x, i64 y) {
  Object *object = spawn(GFX_MONSTER_SLIME, MAP_LAYER_CREATURES, monster->thing.pos.x + x*TILE_PIXELS, monster->thing.pos.y + y*TILE_PIXELS);
  if (object) {
    object->creature.speed = 1.2;
    object->monster.sight = 12;
    if (thing_collides_at(&object->thing, object->thing.pos)) {
      // collided with something, remove it
      object->thing.removed = true;
    }
  }
}

void slime_boss_update(Monster* monster) {
  monster_update(monster);

  if (monster->thing.removed) {
    // game ended
    end_game();
  } else if (riv->frame % 300 == 0) {
    // count monsters
    u64 monsters = 0;
    for (u32 i=1;i<=object_count;++i) {
      Object *object = &objects[i];
      if (!object->thing.removed && object->thing.type == TYPE_MONSTER) {
        monsters++;
      }
    }
    // spawn minions
    if (monsters == 1 && !main_player->thing.removed) {
      slime_boss_spawn_minion(monster,-1, 0);
      slime_boss_spawn_minion(monster, 1, 0);
      slime_boss_spawn_minion(monster, 0, 1);
      slime_boss_spawn_minion(monster, 0,-1);
    }
  }
}

void slime_boss_draw(Monster* monster) {
  riv->draw.pal_enabled = true;
  riv->draw.pal[32] = RIV_COLOR_ORANGE;
  riv->draw.pal[33] = RIV_COLOR_GOLD;
  riv->draw.pal[34] = RIV_COLOR_YELLOW;
  riv->draw.pal[35] = RIV_COLOR_LIGHTYELLOW;
  monster_draw(monster);
  riv->draw.pal[32] = 32;
  riv->draw.pal[33] = 33;
  riv->draw.pal[34] = 34;
  riv->draw.pal[35] = 35;
  riv->draw.pal_enabled = false;
}

//------------------------------------------------------------------------------
// Player

void player_attack_sword(Player *player) {
  i64 scale = player->creature.attack1_damage;
  i64 xflip = player->thing.spr_scale.x;
  vec2 offset = (vec2){xflip*TILE_PIXELS, 0};
  vec2 pos = add_vec2(player->thing.pos, offset);
  if (xflip < 0) {
    pos.x -= TILE_PIXELS*(scale-1);
  }
  if (scale > 0) {
    pos.y -= TILE_PIXELS*(scale-1)/2;
  }
  Object *effect = spawn(GFX_EFFECT_ATTACK, MAP_LAYER_EFFECTS, pos.x, pos.y);
  if (effect) {
    effect->thing.spr_scale.x *= xflip*scale;
    effect->thing.spr_scale.y *= scale;
    effect->thing.bbox = thing_bbox_at(&effect->thing, pos);

    // attack monsters
    recti bbox = expand_recti(effect->thing.bbox, 4);
    Object *object = first_collidable;
    while ((object = thing_collides_with(&player->thing, bbox, TYPE_MONSTER, object))) {
      creature_pull_hit(&object->creature, player->thing.spr_scale.x, player->creature.attack1_damage);
    }
  }
}

void player_attack_bomb(Player *player) {
  Object *bomb = spawn(GFX_ITEM_BOMB, MAP_LAYER_BOTTOM_ITEMS, player->thing.pos.x, player->thing.pos.y);
  if (bomb) {
    bomb->item.damage = player->creature.attack2_damage;
  }
}

void player_update(Player *player) {
  creature_update(&player->creature);
  if (player->thing.removed) { // died
    end_frame = riv->frame;
    return;
  }

  // dash
  i64 dash_ticks = timer_countdown(player->dash_frame, player->dash_delay);
  if (riv->keys[RIV_GAMEPAD_A1].down && dash_ticks == 0) {
    player->dash_frame = riv->frame;
    sfx(SFX_DASH);
    spawn(GFX_EFFECT_DUST, MAP_LAYER_EFFECTS, player->thing.pos.x, player->thing.pos.y);
  }
  bool dashing = dash_ticks > 0;

  // attack
  i64 attack1_ticks = timer_countdown(player->creature.attack1_frame, player->creature.attack1_delay);
  if (player->creature.attack1_damage > 0 && attack1_ticks == 0 && riv->keys[RIV_GAMEPAD_A2].down) {
    player->creature.attack1_frame = riv->frame;
    sfx(SFX_ATTACK);
    player_attack_sword(player);
  }
  i64 attack2_ticks = timer_countdown(player->creature.attack2_frame, player->creature.attack2_delay);
  if (player->creature.attack2_damage > 0 && attack2_ticks == 0 && riv->keys[RIV_GAMEPAD_A3].down) {
    player->creature.attack2_frame = riv->frame;
    player_attack_bomb(player);
  }

  // move
  bool move_horz = riv->keys[RIV_GAMEPAD_RIGHT].down != riv->keys[RIV_GAMEPAD_LEFT].down;
  bool move_vert = riv->keys[RIV_GAMEPAD_DOWN].down != riv->keys[RIV_GAMEPAD_UP].down;
  bool moving = move_horz || move_vert;
  player->creature.moving = moving;
  if (moving) {
    f64 speed = player->creature.speed;
    // increase speed when dashing
    if (dashing) {
      i64 dash_progress = maxi(dash_ticks - (player->dash_delay - player->dash_duration), 0);
      speed *= 1.0 + sqr(player->dash_power*dash_progress/(f64)player->dash_duration);
    }
    // disallow moving faster in diagonals
    if (move_horz && move_vert) {
      speed *= 0.70710678118655; // 1/sqrt(2)
    }
    vec2 delta = {0,0};
    // horizontal move
    if (move_horz) {
      i64 dir = riv->keys[RIV_GAMEPAD_RIGHT].down ? 1 : -1;
      if (dir == player->thing.spr_scale.x) {
        delta.x = speed*dir;
      } else {
        player->thing.spr_scale.x = dir;
      }
    }
    // vertical move
    if (move_vert) {
      i64 dir = riv->keys[RIV_GAMEPAD_DOWN].down ? 1 : -1;
      delta.y = speed*dir;
    }
    creature_move(&player->creature, delta);
  }
}

f64 player_get_dist_sqr(Thing* thing) {
  return distsqr_vec2(main_player->thing.pos, thing->pos);
}

//------------------------------------------------------------------------------
// Spikes

void spikes_update(Item *item) {
  if (thing_collides_with_player(&item->thing) &&
      thing_get_spr_anim(&item->thing, 0) >= GFX_GROUND_SPIKES_HURT &&
      riv->frame >= item->trigger_frame + item->thing.spr_frame_duration * 5) {
    item->trigger_frame = riv->frame;
    creature_hit(&main_player->creature, item->damage);
  }
}

void stairs_update(Item *item) {
  if (thing_collides_with_player(&item->thing)) {
    next_level = level+1;
    if (next_level == NUM_LEVELS) {
      end_game();
    }
  }
}

//------------------------------------------------------------------------------
// Bomb

void bomb_update(Item *item) {
  if ((riv->frame - item->thing.spawn_frame) >= (u64)(item->thing.spr_frame_duration * item->thing.spr_frames)) {
    i64 scale = maxi(item->damage - 1, 1);
    vec2 pos = sub_vec2(item->thing.pos, (vec2){TILE_PIXELS/2, TILE_PIXELS/2});
    if (scale > 0) {
      pos.y -= TILE_PIXELS*(scale-1);
      pos.x -= TILE_PIXELS*(scale-1);
    }
    Object *effect = spawn(GFX_EFFECT_EXPLOSION, MAP_LAYER_EFFECTS, pos.x, pos.y);
    if (effect) {
      effect->thing.spr_scale.x *= scale;
      effect->thing.spr_scale.y *= scale;
      effect->thing.bbox = thing_bbox_at(&effect->thing, pos);

      sfx(SFX_EXPLOSION);

      shake_frame = riv->frame + 20;

      // attack creatures
      recti bbox = expand_recti(effect->thing.bbox, 8);
      Object *object = first_collidable;
      while ((object = thing_collides_with(&item->thing, bbox, TYPE_MONSTER, object))) {
        creature_pull_hit(&object->creature, isign(object->thing.pos.x - item->thing.pos.x), item->damage);
      }
    }
    item->thing.removed = true;
  }
}

void upgrade_bomb_update(Item *item) {
  if (thing_collides_with_player(&item->thing)) {
    sfx(SFX_UPGRADE);
    main_player->creature.attack2_damage = clampi(main_player->creature.attack2_damage+1, 2, 3);
    main_player->creature.attack2_delay = maxi(main_player->creature.attack2_delay-20, 20);
    item->thing.removed = true;

    // find other upgrade and remove it
    Object *other_upgrade = find_object_by_spr(GFX_ITEM_UPGRADE_BLADE);
    if (other_upgrade && distsqr_vec2(other_upgrade->thing.pos, item->thing.pos) <= sqr(TILE_PIXELS*4)) {
      other_upgrade->thing.removed = true;
      spawn(GFX_EFFECT_DUST, MAP_LAYER_EFFECTS, other_upgrade->thing.pos.x, other_upgrade->thing.pos.y);
    }
  } else {
    item->thing.pos = add_vec2(item->thing.spawn_pos, (vec2){0, -3+sin(riv->time*4)*3});
    item->thing.bbox = thing_bbox_at(&item->thing, item->thing.pos);
  }
}

void upgrade_blade_update(Item *item) {
  if (thing_collides_with_player(&item->thing)) {
    sfx(SFX_UPGRADE);
    main_player->creature.attack1_damage += 1;
    item->thing.removed = true;

    // find other upgrade and remove it
    Object *other_upgrade = find_object_by_spr(GFX_ITEM_UPGRADE_BOMB);
    if (other_upgrade && distsqr_vec2(other_upgrade->thing.pos, item->thing.pos) <= sqr(TILE_PIXELS*4)) {
      other_upgrade->thing.removed = true;
      spawn(GFX_EFFECT_DUST, MAP_LAYER_EFFECTS, other_upgrade->thing.pos.x, other_upgrade->thing.pos.y);
    }
  } else {
    item->thing.pos = add_vec2(item->thing.spawn_pos, (vec2){0, -3+sin(3+riv->time*4)*3});
    item->thing.bbox = thing_bbox_at(&item->thing, item->thing.pos);
  }
}

//------------------------------------------------------------------------------
// Coin

void coin_update(Item *item) {
  if (thing_collides_with_player(&item->thing)) {
    sfx(SFX_COIN_PICKUP);
    item->thing.removed = true;
    coins++;
  }
}

//------------------------------------------------------------------------------
// Potion

void potion_update(Item *item) {
  if (thing_collides_with_player(&item->thing)) {
    sfx(SFX_POTION_PICKUP);
    main_player->creature.health = mini(main_player->creature.health + 1, 10);
    item->thing.removed = true;
  }
}

//------------------------------------------------------------------------------
// Key

void key_update(Item *item) {
  if (thing_collides_with_player(&item->thing)) {
    picked_keys++;
    sfx(SFX_KEY_PICKUP);
    item->thing.removed = true;
  }
}

//------------------------------------------------------------------------------
// Chest

void chest_update(Item *item) {
  if (player_get_dist_sqr(&item->thing) <= sqr(TILE_PIXELS*3/2)) {
    item->trigger_frame = riv->frame;
    item->thing.spr = GFX_ITEM_CHEST_OPEN;
    item->thing.spr_frame_duration = 0;
    sfx(SFX_CHEST_OPEN);
  }
}

void open_chest_update(Item *item) {
  switch (riv->frame - item->trigger_frame) {
    case 10:
      sfx(SFX_COIN_DROP);
      spawn(GFX_ITEM_COIN, MAP_LAYER_BOTTOM_ITEMS, item->thing.pos.x-TILE_PIXELS, item->thing.pos.y);
      break;
    case 20:
      sfx(SFX_COIN_DROP);
      spawn(GFX_ITEM_COIN, MAP_LAYER_BOTTOM_ITEMS, item->thing.pos.x+TILE_PIXELS, item->thing.pos.y);
      break;
    case 30:
      sfx(SFX_COIN_DROP);
      spawn(GFX_ITEM_COIN, MAP_LAYER_BOTTOM_ITEMS, item->thing.pos.x, item->thing.pos.y-TILE_PIXELS);
      break;
    case 40:
      sfx(SFX_COIN_DROP);
      spawn(GFX_ITEM_COIN, MAP_LAYER_BOTTOM_ITEMS, item->thing.pos.x, item->thing.pos.y+TILE_PIXELS);
      break;
  }
}

//------------------------------------------------------------------------------
// Doors

void auto_door_update(Item *item) {
  if (item->trigger_frame == 0) {
    if (item->trigger_frame == 0 && player_get_dist_sqr(&item->thing) <= sqr(TILE_PIXELS*3)) {
      item->trigger_frame = riv->frame;
      item->thing.spawn_frame = riv->frame;
      item->thing.spr_frame_duration = 4;
      sfx(SFX_OPEN_DOOR);
    }
  } else if (riv->frame - item->trigger_frame > item->thing.spr_frame_duration &&
               thing_get_spr_anim(&item->thing, 0) == GFX_WALL_AUTO_DOOR) {
    // spawn open door archs
    spawn(GFX_WALL_DOOR_OPEN_TLR, MAP_LAYER_WALLS, item->thing.pos.x, item->thing.pos.y);
    spawn(GFX_WALL_DOOR_OPEN_BL, MAP_LAYER_WALLS, item->thing.pos.x, item->thing.pos.y+TILE_PIXELS);
    spawn(GFX_WALL_DOOR_OPEN_BR, MAP_LAYER_WALLS, item->thing.pos.x+TILE_PIXELS, item->thing.pos.y+TILE_PIXELS);
    // spawn open door ground
    item->thing.removed = true;
  }
}

void closed_door_update(Item *item) {
  if (item->trigger_frame == 0) {
    if (item->trigger_frame == 0 && picked_keys > 0 && player_get_dist_sqr(&item->thing) <= sqr(TILE_PIXELS*3)) {
      item->trigger_frame = riv->frame;
      item->thing.spawn_frame = riv->frame;
      item->thing.spr_frame_duration = 4;
      sfx(SFX_OPEN_DOOR);
    }
  } else if (riv->frame - item->trigger_frame > item->thing.spr_frame_duration &&
               thing_get_spr_anim(&item->thing, 0) == GFX_WALL_CLOSED_DOOR) {
    // spawn open door archs
    spawn(GFX_WALL_DOOR_OPEN_TLR, MAP_LAYER_WALLS, item->thing.pos.x, item->thing.pos.y);
    spawn(GFX_WALL_DOOR_OPEN_BL, MAP_LAYER_WALLS, item->thing.pos.x, item->thing.pos.y+TILE_PIXELS);
    spawn(GFX_WALL_DOOR_OPEN_BR, MAP_LAYER_WALLS, item->thing.pos.x+TILE_PIXELS, item->thing.pos.y+TILE_PIXELS);
    // spawn open door ground
    item->thing.removed = true;
  }
}


//------------------------------------------------------------------------------
// Effect

void effect_update(Effect *effect) {
  effect->thing.removed = riv->frame >= effect->thing.spawn_frame + effect->thing.spr_frames * effect->thing.spr_frame_duration;
}

//------------------------------------------------------------------------------
// Object

void object_update(Object *object) {
  switch(object->thing.spr) {
    case GFX_ITEM_COIN: coin_update(&object->item); break;
    case GFX_ITEM_KEY: key_update(&object->item); break;
    case GFX_ITEM_POTION: potion_update(&object->item); break;
    case GFX_ITEM_CHEST: chest_update(&object->item); break;
    case GFX_ITEM_CHEST_OPEN: open_chest_update(&object->item); break;
    case GFX_ITEM_BOMB: bomb_update(&object->item); break;
    case GFX_ITEM_UPGRADE_BOMB: upgrade_bomb_update(&object->item); break;
    case GFX_ITEM_UPGRADE_BLADE: upgrade_blade_update(&object->item); break;
    case GFX_MONSTER_SLIME_BOSS: slime_boss_update(&object->monster); break;
    case GFX_GROUND_SPIKES: spikes_update(&object->item); break;
    case GFX_GROUND_STAIRS: stairs_update(&object->item); break;
    case GFX_WALL_AUTO_DOOR: auto_door_update(&object->item); break;
    case GFX_WALL_CLOSED_DOOR: closed_door_update(&object->item); break;
    default: {
      switch (object->thing.type) {
        case TYPE_CREATURE: creature_update(&object->creature); break;
        case TYPE_MONSTER: monster_update(&object->monster); break;
        case TYPE_PLAYER: player_update(&object->player); break;
        case TYPE_EFFECT: effect_update(&object->effect); break;
        default: break;
      }
      break;
    }
  }
}

void object_draw(Object *object) {
  switch(object->thing.spr) {
    case GFX_MONSTER_SLIME_BOSS: slime_boss_draw(&object->monster); break;
    default: {
      switch (object->thing.type) {
        case TYPE_MONSTER: monster_draw(&object->monster); break;
        case TYPE_PLAYER:
        case TYPE_CREATURE: creature_draw(&object->creature, 0, 0, 0); break;
        default: thing_draw(&object->thing, 0, 0, 0); break;
      }
    }
  }
}

//------------------------------------------------------------------------------
// Map

Object *spawn(u16 spr, u16 layer, f64 x, f64 y) {
  if (object_count+1 >= MAX_OBJECTS) {
    riv_printf("reached max objects\n");
    return NULL;
  }
  Object *object_base = &gfx_objects[spr];
  if (object_base->thing.removed) { // ignore objects that should not spawn
    return NULL;
  }
  u32 id = object_count;
  Object *object = &objects[id];
  // try to reuse last object, otherwise generate a new one
  if (object_count == 0 || !object->thing.removed || object->thing.type == TYPE_PLAYER) {
    id = ++object_count;
    object = &objects[id];
  }
  *object = *object_base;
  object->thing.id = id;
  object->thing.spawn_pos = (vec2){x,y};
  object->thing.pos = object->thing.spawn_pos;
  object->thing.bbox = thing_bbox_at(&object->thing, object->thing.pos);
  object->thing.layer = layer;
  object->thing.spawn_frame = riv->frame;
  if (object->thing.type == TYPE_NONE) { // define type from layer
    switch (layer) {
    case MAP_LAYER_GROUND: object->thing.type = TYPE_GROUND; break;
    case MAP_LAYER_BOTTOM_ITEMS: object->thing.type = TYPE_ITEM; break;
    case MAP_LAYER_CREATURES: object->thing.type = TYPE_MONSTER; break;
    case MAP_LAYER_WALLS: object->thing.type = TYPE_WALL; break;
    case MAP_LAYER_TOP_ITEMS: object->thing.type = TYPE_ITEM; break;
    }
  }
  return object;
}

recti get_camera_bbox() {
  return (recti){main_player->thing.bbox.x - (SCREEN_PIXELS-TILE_PIXELS)/2,
                 main_player->thing.bbox.y - (SCREEN_PIXELS-TILE_PIXELS)/2,
                 SCREEN_PIXELS, SCREEN_PIXELS};
}

void map_update() {
  recti camera_bbox = get_camera_bbox();
  recti screen_bbox = expand_recti(camera_bbox, TILE_PIXELS*2);
  // update all objects not removed and in screen range
  for (u32 i=1;i<=object_count;++i) {
    Object *object = &objects[i];
    if (!object->thing.removed && overlaps_recti(screen_bbox, object->thing.bbox)) {
      object_update(&objects[i]);
    }
  }
}

void map_draw() {
  riv_clear(RIV_COLOR_DARKSLATE);
  recti camera_bbox = get_camera_bbox();
  recti screen_bbox = expand_recti(camera_bbox, TILE_PIXELS*2);
  riv->draw.origin.x = -camera_bbox.x;
  riv->draw.origin.y = -camera_bbox.y;
  if (shake_frame >= riv->frame) {
    riv->draw.origin.x += riv_rand_uint(3);
    riv->draw.origin.y += riv_rand_uint(3);
  }
  // draw all objects not removed and in screen range
  for (u32 i=1;i<=object_count;++i) {
    Object *object = &objects[i];
    if (!object->thing.removed && overlaps_recti(screen_bbox, object->thing.bbox)) {
      object_draw(&objects[i]);
    }
  }
  riv->draw.origin.x = 0;
  riv->draw.origin.y = 0;
}

//------------------------------------------------------------------------------
// Game

void load_objects_types() {
  for (u32 gfx=1;gfx<NUM_GFX;++gfx) {
    Object *object = &gfx_objects[gfx];
    if (memcmp(object, &gfx_objects[0], sizeof(Object)) == 0) {
      object->thing.spr = gfx;
    }
    if (object->thing.spr_tiles.x == 0) {
      object->thing.spr_tiles = (vec2i){1, 1};
    }
    if (object->thing.spr_scale.x == 0) {
      object->thing.spr_scale = (vec2i){1, 1};
    }
    if (object->thing.spr != 0 && object->thing.spr_bbox.width == 0) {
      object->thing.spr_bbox = riv_get_sprite_bbox(object->thing.spr, SPRITESHEET_GAME, object->thing.spr_tiles.x, object->thing.spr_tiles.y);
    }
  }
}

void load_map(u64 new_level) {
  Player prev_player = (main_player && !main_player->thing.removed) ? *main_player : gfx_objects[GFX_PLAYER].player;
  level = new_level;
  next_level = new_level;
  main_player = NULL;
  first_collidable = NULL;
  object_count = 0;
  picked_keys = 0;
  shake_frame = 0;
  for (u8 l=0;l<NUM_MAP_LAYERS;++l) {
    for (u16 y=0;y<MAP_SIZE;++y) {
      for (u16 x=0;x<MAP_SIZE;++x) {
        u16 gfx = maps[level][l][y][x];
        if (gfx != 0) {
          Object *object = spawn(gfx, l, x * TILE_PIXELS, y * TILE_PIXELS);
          if (object) {
            if (object->thing.type == TYPE_PLAYER) {
              main_player = &object->player;
              prev_player.thing.id = main_player->thing.id;
              prev_player.thing.spawn_pos = main_player->thing.spawn_pos;
              prev_player.thing.pos = main_player->thing.pos;
              prev_player.thing.bbox = main_player->thing.bbox;
              prev_player.thing.layer = main_player->thing.layer;
              prev_player.thing.spawn_frame = main_player->thing.spawn_frame;
              *main_player = prev_player;
            }
            if (l <= MAP_LAYER_BOTTOM_ITEMS && object->thing.type & (TYPE_ITEM | TYPE_GROUND)) {
              first_collidable = object;
            }
          }
        }
      }
    }
  }
  if (!main_player) {
    riv_panic("main player not found");
  }
  riv_printf("LEVEL %d\n", level);
}

void game_init() {
  riv_load_palette("simple_dungeon_crawler_16x16.png", 32);
  riv_make_spritesheet(riv_make_image("simple_dungeon_crawler_16x16.png", 0xff), TILE_PIXELS, TILE_PIXELS);
  load_objects_types();
  load_map(0);

  // load_map(3);
  // main_player->creature.attack2_damage += 3;
  // main_player->creature.attack2_delay -= 20;
  // main_player->creature.attack1_damage += 1;
}

void game_update_score() {
  if (main_player) {
    i64 frames = end_frame > 0 ? end_frame : riv->frame;
    i64 health = main_player->creature.health;
    i64 blade_level = main_player->creature.attack1_damage;
    i64 bomb_level = maxi(main_player->creature.attack2_damage-1, 0);
    i64 score = -frames + level*10000 + health*2000 + coins*2000 + (blade_level + bomb_level)*1000 + kills*200;
    riv->outcard_len = riv_snprintf((char*)riv->outcard, RIV_SIZE_OUTCARD, "JSON{"
      "\"score\":%ld,"
      "\"frames\":%ld,"
      "\"health\":%ld,"
      "\"coins\":%ld,"
      "\"kills\":%ld,"
      "\"level\":%ld,"
      "\"blade_level\":%ld,"
      "\"bomb_level\":%ld"
    "}\n",
      score,frames,health,coins,kills,level+1,blade_level,bomb_level);
  }
}

void game_update() {
  game_update_score();

  if (next_level == NUM_LEVELS) {
    return;
  }
  map_update();

  if (next_level != level && next_level < NUM_LEVELS) {
    load_map(next_level);
  }
}

void draw_bordered_text(const char *text, i64 x, i64 y, i64 col) {
  riv_draw_text(text, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, x-1, y, 1, RIV_COLOR_BLACK);
  riv_draw_text(text, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, x+1, y, 1, RIV_COLOR_BLACK);
  riv_draw_text(text, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, x, y-1, 1, RIV_COLOR_BLACK);
  riv_draw_text(text, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, x, y+1, 1, RIV_COLOR_BLACK);
  riv_draw_text(text, RIV_SPRITESHEET_FONT_5X7, RIV_TOPLEFT, x, y, 1, col);
}

void game_draw() {
  map_draw();
  if (main_player) {
    i32 secs = (end_frame > 0 ? end_frame : riv->frame) / 60;
    i32 health = main_player->creature.health;
    for (i32 i = 0; i < health; ++i) {
      riv_draw_sprite(GFX_ITEM_HEART, SPRITESHEET_GAME, i*13, 4, 1, 1, 1, 1);
    }
    riv_draw_sprite(GFX_ITEM_COIN, SPRITESHEET_GAME, 0, 16, 1, 1, 1, 1);
    riv_draw_sprite(GFX_ITEM_CLOCK, SPRITESHEET_GAME, 256-54, 3, 1, 1, 1, 1);
    draw_bordered_text(riv_tprintf("%d", coins), 16, 14+8, RIV_COLOR_YELLOW);
    draw_bordered_text(riv_tprintf("%02d:%02d", secs / 60, secs % 60), 256-36, 8, RIV_COLOR_WHITE);
  }
  if (main_player && main_player->creature.health == 0) {
    riv_draw_text("GAME OVER", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128-24+1, 2, RIV_COLOR_BLACK);
    riv_draw_text("GAME OVER", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128+1, 128-24, 2, RIV_COLOR_BLACK);
    riv_draw_text("GAME OVER", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128-24, 2, RIV_COLOR_RED + (riv->frame / 8) % 3);
  } else if (next_level == NUM_LEVELS) {
    riv_draw_text("GAME COMPLETED", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128-24+2, 2, RIV_COLOR_BLACK);
    riv_draw_text("GAME COMPLETED", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128+2, 128-24, 2, RIV_COLOR_BLACK);
    riv_draw_text("GAME COMPLETED", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128-24, 2, RIV_COLOR_YELLOW + (riv->frame / 8) % 2);
    riv_draw_text("  THANKS FOR PLAYING!\n\n      a game by\n edubart and isabella", RIV_SPRITESHEET_FONT_5X7, RIV_CENTER, 128, 128+48, 1, RIV_COLOR_YELLOW);
  }
}

//------------------------------------------------------------------------------
// Main

int main() {
  game_init();
  do {
    game_update();
    game_draw();
  } while(riv_present());
}
