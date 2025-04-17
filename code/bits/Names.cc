#include "Names.h"

#include <cassert>

#include <algorithm>
#include <string_view>

namespace ffw {

  namespace {
    constexpr std::size_t NameLengthMax = 22;
    constexpr double MiddleNameProbability = 0.2;

    // data taken from https://www.census.gov/topics/population/genealogy/data/1990_census/1990_census_namefiles.html
    // only the first 200 entries were kept for each file

    constexpr std::string_view Surnames[] = {
      "Adams", "Alexander", "Allen", "Anderson", "Andrews", "Armstrong", "Arnold", "Austin", "Bailey", "Baker",
      "Barnes", "Bell", "Bennett", "Berry", "Black", "Boyd", "Bradley", "Brooks", "Brown", "Bryant",
      "Burns", "Butler", "Campbell", "Carpenter", "Carroll", "Carter", "Chavez", "Clark", "Cole", "Coleman",
      "Collins", "Cook", "Cooper", "Cox", "Crawford", "Cruz", "Cunningham", "Daniels", "Davis", "Diaz",
      "Dixon", "Duncan", "Dunn", "Edwards", "Elliott", "Ellis", "Evans", "Ferguson", "Fisher", "Flores",
      "Ford", "Foster", "Fox", "Franklin", "Freeman", "Garcia", "Gardner", "Gibson", "Gomez", "Gonzales",
      "Gonzalez", "Gordon", "Graham", "Grant", "Gray", "Green", "Greene", "Griffin", "Hall", "Hamilton",
      "Harper", "Harris", "Harrison", "Hart", "Hawkins", "Hayes", "Henderson", "Henry", "Hernandez", "Hicks",
      "Hill", "Holmes", "Howard", "Hudson", "Hughes", "Hunt", "Hunter", "Jackson", "James", "Jenkins",
      "Johnson", "Jones", "Jordan", "Kelley", "Kelly", "Kennedy", "King", "Knight", "Lane", "Lawrence",
      "Lawson", "Lee", "Lewis", "Long", "Lopez", "Marshall", "Martin", "Martinez", "Mason", "Matthews",
      "Mcdonald", "Miller", "Mills", "Mitchell", "Moore", "Morales", "Morgan", "Morris", "Murphy", "Murray",
      "Myers", "Nelson", "Nichols", "Olson", "Ortiz", "Owens", "Palmer", "Parker", "Patterson", "Payne",
      "Perez", "Perkins", "Perry", "Peters", "Peterson", "Phillips", "Pierce", "Porter", "Powell", "Price",
      "Ramirez", "Ramos", "Ray", "Reed", "Reyes", "Reynolds", "Rice", "Richardson", "Riley", "Rivera",
      "Roberts", "Robertson", "Robinson", "Rodriguez", "Rogers", "Rose", "Ross", "Ruiz", "Russell", "Sanchez",
      "Sanders", "Scott", "Shaw", "Simmons", "Simpson", "Sims", "Smith", "Snyder", "Spencer", "Stephens",
      "Stevens", "Stewart", "Stone", "Sullivan", "Taylor", "Thomas", "Thompson", "Torres", "Tucker", "Turner",
      "Wagner", "Walker", "Wallace", "Ward", "Warren", "Washington", "Watkins", "Watson", "Weaver", "Webb",
      "Wells", "West", "White", "Williams", "Willis", "Wilson", "Wood", "Woods", "Wright", "Young",
    };

    constexpr std::string_view FemaleNames[] = {
      "Alice", "Alicia", "Alma", "Amanda", "Amber", "Amy", "Ana", "Andrea", "Angela", "Anita",
      "Ann", "Anna", "Anne", "Annette", "Annie", "April", "Ashley", "Audrey", "Barbara", "Beatrice",
      "Bernice", "Bertha", "Beth", "Betty", "Beverly", "Bonnie", "Brenda", "Brittany", "Carmen", "Carol",
      "Carolyn", "Carrie", "Catherine", "Cathy", "Charlotte", "Cheryl", "Christina", "Christine", "Cindy", "Clara",
      "Connie", "Crystal", "Cynthia", "Dana", "Danielle", "Darlene", "Dawn", "Debbie", "Deborah", "Debra",
      "Denise", "Diana", "Diane", "Dolores", "Donna", "Doris", "Dorothy", "Edith", "Edna", "Elaine",
      "Eleanor", "Elizabeth", "Ellen", "Elsie", "Emily", "Emma", "Erica", "Erin", "Esther", "Ethel",
      "Eva", "Evelyn", "Florence", "Frances", "Gail", "Geraldine", "Gladys", "Gloria", "Grace", "Hazel",
      "Heather", "Helen", "Holly", "Ida", "Irene", "Jacqueline", "Jamie", "Jane", "Janet", "Janice",
      "Jean", "Jeanette", "Jeanne", "Jennifer", "Jessica", "Jill", "Joan", "Joann", "Joanne", "Josephine",
      "Joyce", "Juanita", "Judith", "Judy", "Julia", "Julie", "June", "Karen", "Katherine", "Kathleen",
      "Kathryn", "Kathy", "Katie", "Kelly", "Kim", "Kimberly", "Kristen", "Laura", "Lauren", "Laurie",
      "Leslie", "Lillian", "Linda", "Lisa", "Lois", "Loretta", "Lori", "Lorraine", "Louise", "Lucille",
      "Lynn", "Margaret", "Maria", "Marie", "Marilyn", "Marion", "Marjorie", "Martha", "Mary", "Megan",
      "Melanie", "Melissa", "Michele", "Michelle", "Mildred", "Monica", "Nancy", "Nicole", "Norma", "Pamela",
      "Patricia", "Paula", "Pauline", "Peggy", "Phyllis", "Rachel", "Rebecca", "Regina", "Renee", "Rhonda",
      "Rita", "Roberta", "Robin", "Rosa", "Rose", "Ruby", "Ruth", "Sally", "Samantha", "Sandra",
      "Sara", "Sarah", "Shannon", "Sharon", "Sheila", "Sherry", "Shirley", "Stacy", "Stephanie", "Sue",
      "Susan", "Suzanne", "Sylvia", "Tammy", "Teresa", "Thelma", "Theresa", "Tiffany", "Tina", "Tracy",
      "Valerie", "Vanessa", "Veronica", "Victoria", "Virginia", "Vivian", "Wanda", "Wendy", "Yolanda", "Yvonne",
    };

    constexpr std::string_view MaleNames[] = {
      "Aaron", "Adam", "Alan", "Albert", "Alex", "Alexander", "Alfred", "Allen", "Alvin", "Andrew",
      "Anthony", "Antonio", "Arthur", "Barry", "Benjamin", "Bernard", "Bill", "Billy", "Bobby", "Bradley",
      "Brandon", "Brent", "Brian", "Bruce", "Bryan", "Calvin", "Carl", "Carlos", "Chad", "Charles",
      "Charlie", "Chris", "Christopher", "Clarence", "Clifford", "Clyde", "Corey", "Craig", "Curtis", "Dale",
      "Dan", "Daniel", "Danny", "Darrell", "David", "Dean", "Dennis", "Derek", "Derrick", "Don",
      "Donald", "Douglas", "Dustin", "Earl", "Eddie", "Edward", "Edwin", "Eric", "Ernest", "Eugene",
      "Floyd", "Francis", "Francisco", "Frank", "Fred", "Frederick", "Gary", "Gene", "George", "Gerald",
      "Gilbert", "Glen", "Glenn", "Gordon", "Greg", "Gregory", "Harold", "Harry", "Hector", "Henry",
      "Herbert", "Herman", "Howard", "Jack", "Jacob", "James", "Jason", "Jay", "Jeff", "Jeffery",
      "Jeffrey", "Jeremy", "Jerome", "Jerry", "Jesse", "Jesus", "Jim", "Jimmy", "Joe", "Joel",
      "John", "Johnny", "Jon", "Jonathan", "Jorge", "Jose", "Joseph", "Joshua", "Juan", "Justin",
      "Keith", "Kenneth", "Kevin", "Kyle", "Larry", "Lawrence", "Lee", "Leo", "Leon", "Leonard",
      "Leroy", "Lester", "Lewis", "Lloyd", "Louis", "Luis", "Manuel", "Marcus", "Mario", "Mark",
      "Martin", "Marvin", "Matthew", "Maurice", "Melvin", "Michael", "Micheal", "Miguel", "Mike", "Nathan",
      "Nicholas", "Norman", "Oscar", "Patrick", "Paul", "Pedro", "Peter", "Philip", "Phillip", "Ralph",
      "Ramon", "Randall", "Randy", "Ray", "Raymond", "Ricardo", "Richard", "Rick", "Ricky", "Robert",
      "Roberto", "Rodney", "Roger", "Ronald", "Ronnie", "Roy", "Russell", "Ryan", "Sam", "Samuel",
      "Scott", "Sean", "Shane", "Shawn", "Stanley", "Stephen", "Steve", "Steven", "Terry", "Theodore",
      "Thomas", "Tim", "Timothy", "Todd", "Tom", "Tommy", "Tony", "Travis", "Troy", "Tyler",
      "Vernon", "Victor", "Vincent", "Walter", "Warren", "Wayne", "Wesley", "William", "Willie", "Zachary",
    };

  }

  std::string generate_random_male_name(gf::Random* random)
  {
    std::string name;

    while (name.empty() || name.length() > NameLengthMax) {
      name.clear();

      const std::size_t first_name_index = random->compute_uniform_integer(std::size(MaleNames));
      assert(first_name_index < std::size(MaleNames));

      const std::size_t surname_index = random->compute_uniform_integer(std::size(Surnames));
      assert(surname_index < std::size(Surnames));

      name += MaleNames[first_name_index];
      name += ' ';

      if (random->compute_bernoulli(MiddleNameProbability)) {
        name += random->compute_uniform_integer('A', '['); // A .. Z
        name += ". ";
      }

      name += Surnames[surname_index];
    }

    return name;
  }

  std::string generate_random_female_name(gf::Random* random)
  {
    std::string name;

    while (name.empty() || name.length() > NameLengthMax) {
      name.clear();

      const std::size_t first_name_index = random->compute_uniform_integer(std::size(FemaleNames));
      assert(first_name_index < std::size(FemaleNames));

      const std::size_t surname_index = random->compute_uniform_integer(std::size(Surnames));
      assert(surname_index < std::size(Surnames));

      name += FemaleNames[first_name_index];
      name += ' ';

      if (random->compute_bernoulli(MiddleNameProbability)) {
        name += random->compute_uniform_integer('A', '['); // A .. Z
        name += ". ";
      }

      name += Surnames[surname_index];
    }

    return name;
  }

  std::size_t compute_max_length(NameType type)
  {
    auto compare = [](std::string_view lhs, std::string_view rhs) { return lhs.length() < rhs.length(); };

    switch (type) {
      case NameType::MaleName:
        return std::max_element(std::begin(MaleNames), std::end(MaleNames), compare)->length();
      case NameType::FemaleName:
        return std::max_element(std::begin(FemaleNames), std::end(FemaleNames), compare)->length();
      case NameType::Surname:
        return std::max_element(std::begin(Surnames), std::end(Surnames), compare)->length();
    }

    return 0;
  }


}
