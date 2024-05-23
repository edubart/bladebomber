{
  // player
  [GFX_PLAYER] = {
    .player = {
      .creature.thing.type = TYPE_PLAYER,
      .creature.thing.spr = GFX_PLAYER,
      .creature.thing.spr_bbox = {2, 0, 12, 15},
      .creature.thing.spr_frames = 6,
      .creature.thing.spr_frame_duration = 4,
      .creature.speed = 4/3.0,
      .creature.health = 5,
      .creature.attack1_delay = 20,
      .creature.attack1_damage = 0,
      .creature.attack2_delay = 80,
      .creature.attack2_damage = 0,
      .dash_delay = 30,
      .dash_duration = 20,
      .dash_power = 2.0,
    },
  },
  // grounds
  [GFX_GROUND_SPIKES] = {
    .item = {
      .thing.type = TYPE_GROUND,
      .thing.spr = GFX_GROUND_SPIKES,
      .thing.spr_frames = 10,
      .thing.spr_frame_duration = 6,
      .thing.spr_loop_delay = 8,
      .damage = 1,
    },
  },
  // auto open door
  [GFX_WALL_AUTO_DOOR] = {
      .thing = {
        .type = TYPE_WALL,
        .spr = GFX_WALL_AUTO_DOOR,
        .spr_bbox = {0, 9, 32, 23},
        .spr_tiles = {2, 2},
        .spr_frames = 11,
        .spr_frame_duration = 0,
      },
  },
  [GFX_WALL_AUTO_DOOR+1] = {.thing.removed = true},
  [GFX_WALL_AUTO_DOOR+16] = {.thing.removed = true},
  [GFX_WALL_AUTO_DOOR+17] = {.thing.removed = true},
  [GFX_WALL_CLOSED_DOOR] = {
      .thing = {
        .type = TYPE_WALL,
        .spr = GFX_WALL_CLOSED_DOOR,
        .spr_bbox = {0, 9, 32, 23},
        .spr_tiles = {2, 2},
        .spr_frames = 14,
        .spr_frame_duration = 0,
      },
  },
  // closed door
  [GFX_WALL_CLOSED_DOOR+1] = {.thing.removed = true},
  [GFX_WALL_CLOSED_DOOR+16] = {.thing.removed = true},
  [GFX_WALL_CLOSED_DOOR+17] = {.thing.removed = true},
  // arch
  [GFX_WALL_ARCH] = {
    .thing = {
      .type = TYPE_WALL,
      .spr = GFX_WALL_ARCH,
      .spr_tiles = {2, 1},
      .phantom = true,
    },
  },
  [GFX_WALL_ARCH+1] = {.thing.removed = true},
  [GFX_WALL_DOOR_OPEN_TLR] = {
    .thing = {
      .type = TYPE_WALL,
      .spr = GFX_WALL_DOOR_OPEN_TLR,
      .spr_tiles = {2, 1},
      .phantom = true,
    },
  },
  // items
  [GFX_ITEM_TORCH] = {
    .item = {
      .thing.type = TYPE_ITEM,
      .thing.spr = GFX_ITEM_TORCH,
      .thing.spr_frames = 6,
      .thing.spr_frame_duration = 4,
    },
  },
  [GFX_ITEM_CHEST] = {
    .item = {
      .thing.type = TYPE_WALL,
      .thing.spr = GFX_ITEM_CHEST,
      .thing.spr_frames = 8,
      .thing.spr_frame_duration = 6,
      .thing.spr_loop_delay = 12,
    },
  },
  [GFX_ITEM_BOMB] = {
    .item = {
      .thing.type = TYPE_ITEM,
      .thing.spr = GFX_ITEM_BOMB,
      .thing.spr_frames = 9,
      .thing.spr_frame_duration = 8,
      .damage = 2,
    },
  },
  // monsters
  [GFX_MONSTER_BAT] = {
    .monster = {
      .creature.thing.type = TYPE_MONSTER,
      .creature.thing.spr = GFX_MONSTER_BAT,
      .creature.thing.spr_bbox = {2, 4, 12, 10},
      .creature.thing.spr_frames = 4,
      .creature.thing.spr_frame_duration = 4,
      .creature.speed = 0.8,
      .creature.health = 2,
      .creature.attack1_delay = 30,
      .creature.attack1_damage = 1,
      .sight = 6,
    },
  },
  [GFX_MONSTER_GOBLIN] = {
    .monster = {
      .creature.thing.type = TYPE_MONSTER,
      .creature.thing.spr = GFX_MONSTER_GOBLIN,
      .creature.thing.spr_bbox = {1, 0, 15, 14},
      .creature.thing.spr_frames = 5,
      .creature.thing.spr_frame_duration = 4,
      .creature.speed = 0.9,
      .creature.health = 4,
      .creature.attack1_delay = 30,
      .creature.attack1_damage = 1,
      .creature.moving = true,
      .sight = 7,
    },
  },
  [GFX_MONSTER_SLIME] = {
    .monster = {
      .creature.thing.type = TYPE_MONSTER,
      .creature.thing.spr = GFX_MONSTER_SLIME,
      .creature.thing.spr_bbox = {0, 3, 16, 12},
      .creature.thing.spr_frames = 4,
      .creature.thing.spr_frame_duration = 4,
      .creature.speed = 0.75,
      .creature.health = 8,
      .creature.attack1_delay = 30,
      .creature.attack1_damage = 1,
      .sight = 8,
    },
  },
  [GFX_MONSTER_SLIME_BOSS] = {
    .monster = {
      .creature.thing.type = TYPE_MONSTER,
      .creature.thing.spr = GFX_MONSTER_SLIME_BOSS,
      .creature.thing.spr_bbox = {0, 3, 16, 12},
      .creature.thing.spr_scale = {1, 1},
      .creature.thing.spr_frames = 4,
      .creature.thing.spr_frame_duration = 3,
      .creature.speed = 1,
      .creature.health = 80,
      .creature.attack1_delay = 30,
      .creature.attack1_damage = 1,
      .sight = 12,
    },
  },
  // effects
  [GFX_EFFECT_DUST] = {
    .effect = {
      .thing.type = TYPE_EFFECT,
      .thing.spr = GFX_EFFECT_DUST,
      .thing.spr_bbox = {0, 0, 16, 16},
      .thing.spr_frames = 3,
      .thing.spr_frame_duration = 4,
    },
  },
  [GFX_EFFECT_ATTACK] = {
    .effect = {
      .thing.type = TYPE_EFFECT,
      .thing.spr = GFX_EFFECT_ATTACK,
      .thing.spr_bbox = {0, 0, 12, 16},
      .thing.spr_frames = 3,
      .thing.spr_frame_duration = 4,
    },
  },
  [GFX_EFFECT_EXPLOSION] = {
    .effect = {
      .thing.type = TYPE_EFFECT,
      .thing.spr = GFX_EFFECT_EXPLOSION,
      .thing.spr_bbox = {0, 0, 32, 32},
      .thing.spr_tiles = {2, 2},
      .thing.spr_frames = 7,
      .thing.spr_frame_duration = 4,
    },
  },
};
