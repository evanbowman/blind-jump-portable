#include "game.hpp"
#include "script/lisp.hpp"


// We're storing a pointer to the C++ game class instance in a userdata object,
// so that routines registered with the lisp interpreter can change game state.
static Game* interp_get_game()
{
    auto game = lisp::get_var("*game*");
    if (game->type_ not_eq lisp::Value::Type::user_data) {
        return nullptr;
    }
    return (Game*)game->user_data_.obj_;
}


static Platform* interp_get_pfrm()
{
    auto pfrm = lisp::get_var("*pfrm*");
    if (pfrm->type_ not_eq lisp::Value::Type::user_data) {
        return nullptr;
    }
    return (Platform*)pfrm->user_data_.obj_;
}


static Entity* get_entity_by_id(Game& game, Entity::Id id)
{
    if (id == game.player().id()) {
        return &game.player();
    }
    if (id == game.transporter().id()) {
        return &game.transporter();
    }

    Entity* result = nullptr;

    auto match_id = [&](auto& buf) {
        for (auto& entity : buf) {
            if (entity->id() == id) {
                result = entity.get();
            }
        }
    };

    game.effects().transform(match_id);
    game.details().transform(match_id);
    game.enemies().transform(match_id);

    return result;
}


void Game::init_script(Platform& pfrm)
{
    lisp::init(pfrm);

    lisp::set_var("*game*", lisp::make_userdata(this));

    lisp::set_var("detail",
                  lisp::make_function([](int argc) { return L_NIL; }));

    lisp::set_var("level", lisp::make_function([](int argc) {
                      if (auto game = interp_get_game()) {
                          if (argc == 1) {
                              L_EXPECT_OP(0, integer);

                              game->persistent_data().level_
                                  .set(lisp::get_op(0)->integer_.value_);

                          } else {
                              return lisp::make_integer(
                                  game->persistent_data().level_.get());
                          }
                      }
                      return L_NIL;
                  }));

    lisp::set_var("add-items", lisp::make_function([](int argc) {
                      if (auto game = interp_get_game()) {
                          for (int i = 0; i < argc; ++i) {
                              const auto item = static_cast<Item::Type>(
                                  lisp::get_op(i)->integer_.value_);

                              if (auto pfrm = interp_get_pfrm()) {
                                  game->inventory().push_item(
                                      *pfrm, *game, item, false);
                              }
                          }
                      }

                      return L_NIL;
                  }));

    lisp::set_var("set-hp", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 2);
                      L_EXPECT_OP(0, integer);
                      L_EXPECT_OP(1, integer);

                      auto game = interp_get_game();
                      if (not game) {
                          return L_NIL;
                      }

                      auto entity = get_entity_by_id(
                          *game, lisp::get_op(1)->integer_.value_);
                      if (entity) {
                          entity->set_health(lisp::get_op(0)->integer_.value_);
                      }

                      return L_NIL;
                  }));

    lisp::set_var(
        "kill", lisp::make_function([](int argc) {
            auto game = interp_get_game();
            if (not game) {
                return L_NIL;
            }

            for (int i = 0; i < argc; ++i) {

                if (lisp::get_op(i)->type_ not_eq lisp::Value::Type::integer) {
                    const auto err = lisp::Error::Code::invalid_argument_type;
                    return lisp::make_error(err);
                }

                auto entity =
                    get_entity_by_id(*game, lisp::get_op(i)->integer_.value_);

                if (entity) {
                    entity->set_health(0);
                }
            }

            return L_NIL;
        }));

    lisp::set_var("get-pos", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 1);
                      L_EXPECT_OP(0, integer);

                      auto game = interp_get_game();
                      if (not game) {
                          return L_NIL;
                      }

                      auto entity = get_entity_by_id(
                          *game, lisp::get_op(0)->integer_.value_);
                      if (not entity) {
                          return L_NIL;
                      }

                      return lisp::make_cons(
                          lisp::make_integer(entity->get_position().x),
                          lisp::make_integer(entity->get_position().y));
                  }));

    lisp::set_var(
        "enemies", lisp::make_function([](int argc) {
            auto game = interp_get_game();
            if (not game) {
                return L_NIL;
            }

            lisp::Value* lat = L_NIL;

            game->enemies().transform([&](auto& buf) {
                for (auto& enemy : buf) {
                    lisp::push_op(lat); // To keep it from being collected
                    lat = lisp::make_cons(L_NIL, lat);
                    lisp::pop_op(); // lat

                    lisp::push_op(lat);
                    lat->cons_.set_car(lisp::make_integer(enemy->id()));
                    lisp::pop_op(); // lat
                }
            });

            return lat;
        }));

    lisp::set_var("log-severity", lisp::make_function([](int argc) {
                      auto game = interp_get_game();
                      if (not game) {
                          return L_NIL;
                      }

                      if (argc == 0) {
                          const auto sv =
                              game->persistent_data().settings_.log_severity_;
                          return lisp::make_integer(static_cast<int>(sv));
                      } else {
                          L_EXPECT_ARGC(argc, 1);
                          L_EXPECT_OP(0, integer);

                          auto val = static_cast<Severity>(
                              lisp::get_op(0)->integer_.value_);
                          game->persistent_data().settings_.log_severity_ = val;
                      }

                      return L_NIL;
                  }));

    lisp::set_var("register-controller", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 7);

                      for (int i = 0; i < 7; ++i) {
                          L_EXPECT_OP(i, integer);
                      }

                      auto pfrm = interp_get_pfrm();
                      if (not pfrm) {
                          return L_NIL;
                      }

                      pfrm->keyboard().register_controller(
                          {lisp::get_op(6)->integer_.value_,
                           lisp::get_op(5)->integer_.value_,
                           lisp::get_op(4)->integer_.value_,
                           lisp::get_op(3)->integer_.value_,
                           lisp::get_op(2)->integer_.value_,
                           lisp::get_op(1)->integer_.value_,
                           lisp::get_op(0)->integer_.value_});

                      return L_NIL;
                  }));

    lisp::set_var("platform", lisp::make_function([](int argc) {
                      auto pfrm = interp_get_pfrm();
                      if (not pfrm) {
                          return L_NIL;
                      }
                      return lisp::make_symbol(pfrm->device_name().c_str());
                  }));

    lisp::set_var("set-tile", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 4);
                      for (int i = 0; i < 4; ++i) {
                          L_EXPECT_OP(i, integer);
                      }

                      auto pfrm = interp_get_pfrm();
                      if (not pfrm) {
                          return L_NIL;
                      }

                      pfrm->set_tile((Layer)lisp::get_op(3)->integer_.value_,
                                     lisp::get_op(2)->integer_.value_,
                                     lisp::get_op(1)->integer_.value_,
                                     lisp::get_op(0)->integer_.value_);

                      return L_NIL;
                  }));

    lisp::set_var("get-tile", lisp::make_function([](int argc) {
                      L_EXPECT_ARGC(argc, 3);
                      for (int i = 0; i < 3; ++i) {
                          L_EXPECT_OP(i, integer);
                      }

                      auto pfrm = interp_get_pfrm();
                      if (not pfrm) {
                          return L_NIL;
                      }

                      auto tile = pfrm->get_tile(
                          (Layer)lisp::get_op(2)->integer_.value_,
                          lisp::get_op(1)->integer_.value_,
                          lisp::get_op(0)->integer_.value_);

                      return lisp::make_integer(tile);
                  }));

    lisp::set_var(
        "pattern-replace-tile", lisp::make_function([](int argc) {
            L_EXPECT_ARGC(argc, 2);
            L_EXPECT_OP(1, integer);
            L_EXPECT_OP(0, cons);
            lisp::Value* filter[3][3];

            auto row = lisp::get_op(0);
            for (int y = 0; y < 3; ++y) {
                if (row->type_ not_eq lisp::Value::Type::cons) {
                    while (true)
                        ;
                }

                auto cell = row->cons_.car();
                for (int x = 0; x < 3; ++x) {
                    if (cell->type_ not_eq lisp::Value::Type::cons) {
                        while (true)
                            ;
                    }

                    filter[x][y] = cell->cons_.car();

                    cell = cell->cons_.cdr();
                }

                row = row->cons_.cdr();
            }

            auto pfrm = interp_get_pfrm();
            if (not pfrm) {
                return L_NIL;
            }

            for (int x = 0; x < TileMap::width; ++x) {
                for (int y = 0; y < TileMap::height; ++y) {
                    bool match = true;
                    for (int xx = -1; xx < 2; ++xx) {
                        for (int yy = -1; yy < 2; ++yy) {
                            int filter_x = xx + 1;
                            int filter_y = yy + 1;

                            auto fval = filter[filter_x][filter_y];
                            if (fval->type_ == lisp::Value::Type::integer) {
                                if (fval->integer_.value_ not_eq
                                    (int) pfrm->get_tile(
                                        Layer::map_0, x + xx, y + yy)) {
                                    match = false;
                                    goto DONE;
                                }
                            } else if (fval->type_ == lisp::Value::Type::cons) {
                                bool submatch = false;
                                while (fval not_eq L_NIL) {
                                    if (fval->type_ not_eq
                                        lisp::Value::Type::cons) {
                                        return L_NIL;
                                    }

                                    if (fval->cons_.car()->type_ not_eq
                                        lisp::Value::Type::integer) {
                                        return L_NIL;
                                    }

                                    if (fval->cons_.car()->integer_.value_ ==
                                        (int)pfrm->get_tile(
                                            Layer::map_0, x + xx, y + yy)) {
                                        submatch = true;
                                    }

                                    fval = fval->cons_.cdr();
                                }
                                if (not submatch) {
                                    match = false;
                                    goto DONE;
                                }
                            }
                        }
                    }
                DONE:
                    if (match) {
                        pfrm->set_tile(Layer::map_0,
                                       x,
                                       y,
                                       lisp::get_op(1)->integer_.value_);
                    }
                }
            }

            return L_NIL;
        }));

#define L_ITEM_K(NAME) \
    {"item-" #NAME, (int)Item::Type::NAME}

    static constexpr const lisp::IntegralConstant constant_table[] = {
        {"boss-0-level", boss_0_level},
        {"boss-1-level", boss_1_level},
        {"boss-2-level", boss_2_level},
        {"player", 0},
        L_ITEM_K(worker_notebook_1),
        L_ITEM_K(blaster),
        L_ITEM_K(accelerator),
        L_ITEM_K(lethargy),
        L_ITEM_K(old_poster_1),
        L_ITEM_K(map_system),
        L_ITEM_K(explosive_rounds_2),
        L_ITEM_K(seed_packet),
        L_ITEM_K(engineer_notebook_2),
        L_ITEM_K(signal_jammer),
        L_ITEM_K(navigation_pamphlet),
        L_ITEM_K(orange),
        L_ITEM_K(orange_seeds),
        L_ITEM_K(long_jump_z2),
        L_ITEM_K(long_jump_z3),
        L_ITEM_K(long_jump_z4),
        L_ITEM_K(engineer_notebook_1),
        L_ITEM_K(worker_notebook_2)};

    const auto constant_count =
        sizeof constant_table / sizeof(lisp::IntegralConstant);

    lisp::set_constants(constant_table, constant_count);
}
