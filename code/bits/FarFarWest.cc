#include "FarFarWest.h"

#include <filesystem>
#include <gf2/core/Log.h>

#include "WorldGeneration.h"
#include "Settings.h"

namespace ffw {

  namespace {

    constexpr gf::ConsoleStyle DefaultStyle = { {  gf::White, gf::Transparent }, gf::ConsoleEffect::set() };

    constexpr gf::ConsoleColorStyle Gf = { gf::Orange, gf::Azure };

    constexpr gf::ConsoleColorStyle DateStyle = { gf::Amber, gf::Transparent };
    constexpr gf::ConsoleColorStyle CharacterStyle = { gf::Chartreuse, gf::Transparent };
    constexpr gf::ConsoleColorStyle ItemStyle = { gf::Capri, gf::Transparent };
    constexpr gf::ConsoleColorStyle CashStyle = { gf::Erin, gf::Transparent };
    constexpr gf::ConsoleColorStyle DebtStyle = { gf::Vermilion, gf::Transparent };
    constexpr gf::ConsoleColorStyle StatStyle = { gf::Azure, gf::Transparent };

    gf::ConsoleRichStyle compute_rich_style()
    {
      gf::ConsoleRichStyle style;

      style.set_default_style(DefaultStyle);

      style.set_style("gf", Gf);

      style.set_style("character", CharacterStyle);
      style.set_style("date", DateStyle);
      style.set_style("item", ItemStyle);
      style.set_style("cash", CashStyle);
      style.set_style("debt", DebtStyle);
      style.set_style("stat", StatStyle);

      return style;
    }

  }

  FarFarWest::FarFarWest(gf::Random* random, const std::filesystem::path& datafile, const std::filesystem::path& savefile)
  : gf::ConsoleSceneManager(ConsoleSize)
  , title(this)
  , kickoff(this)
  , generation(this)
  , primary(this)
  , control(this)
  , quit(this)
  , m_random(random)
  , m_datafile(datafile)
  , m_savefile(savefile)
  , m_model(random)
  , m_rich_style(compute_rich_style())
  {
    push_scene(&title);
    push_scene(&kickoff);
  }

  void FarFarWest::start_world_generation(AdventureChoice choice)
  {
    m_async_generation_finished = false;

    m_async_generation = std::async(std::launch::async, [&,choice]() {
      m_model.data.load_from_file(m_datafile);

      if (choice == AdventureChoice::New) {
        m_model.state = generate_world(m_random);
      } else {
        assert(has_save());
        m_model.state.load_from_file(m_savefile);
        std::filesystem::remove(m_savefile);
      }

      m_model.bind();
    });
  }

  bool FarFarWest::world_generation_finished()
  {
    if (m_async_generation.valid() && m_async_generation.wait_for(std::chrono::seconds::zero()) == std::future_status::ready) {
      m_async_generation.get();
      m_async_generation_finished = true;
    }

    return m_async_generation_finished;
  }

  void FarFarWest::start_world()
  {
    pop_all_scenes();
    push_scene(&primary);
    push_scene(&control);
  }

  bool FarFarWest::has_save() const
  {
    return std::filesystem::is_regular_file(m_savefile);
  }

  void FarFarWest::save()
  {
    m_model.state.save_to_file(m_savefile);
  }


}
