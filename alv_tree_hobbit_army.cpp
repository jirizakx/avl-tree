#include <cassert>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <memory>
#include <limits>
#include <optional>
#include <algorithm>
#include <functional>
#include <bitset>
#include <list>
#include <array>
#include <vector>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <random>
#include <type_traits>
#include <utility>

struct Hobbit {
  std::string name;
  int hp, off, def;

  friend bool operator == (const Hobbit&, const Hobbit&) = default;
};

std::ostream& operator << (std::ostream& out, const Hobbit& h) {
  return out
    << "Hobbit{\"" << h.name << "\", "
    << ".hp=" << h.hp << ", "
    << ".off=" << h.off << ", "
    << ".def=" << h.def << "}";
}

template < typename T >
std::ostream& operator << (std::ostream& out, const std::optional<T>& x) {
  if (!x) return out << "EMPTY_OPTIONAL";
  return out << "Optional{" << *x << "}";
}

struct PendingEnchant {
  PendingEnchant(int hp = 0, int off = 0, int def = 0) : hp(hp), off(off), def(def) {}
  void add(const PendingEnchant& e) {
    hp += e.hp;
    off += e.off;
    def += e.def;
  }
  void clear() {
    hp = 0;
    off = 0;
    def = 0;
  }

  bool empty() const {
    return !hp && !off && !def;
  }
  int hp, off, def;
};

struct Node {
  Node(const Hobbit& hobbit) : left(nullptr), right(nullptr), height(1), hobbit(hobbit)  {}
  std::unique_ptr<Node> left;
  std::unique_ptr<Node> right;
  int height; // absolute heigth
  Hobbit hobbit;
  PendingEnchant pending;
};

struct HobbitArmy {
  static constexpr bool CHECK_NEGATIVE_HP = true;
  std::unique_ptr<Node> root;

  bool add(const Hobbit& hobbit) {
    if (hobbit.hp < 1) return false;
    bool success = true;
    root = add_impl(hobbit, std::move(root), success);
    return success;
  }

  std::unique_ptr<Node> add_impl(const Hobbit& hobbit, std::unique_ptr<Node> curr, bool& success) {
    if (!curr) {
      success = true;
      return std::make_unique<Node>(hobbit);
    } 
    push_enchant(curr.get());
    
    if (curr->hobbit.name == hobbit.name) {
      success = false; // this hobbit already exists
      return curr;
    }

    if (curr->hobbit.name > hobbit.name) curr->left = add_impl(hobbit, std::move(curr->left), success);
    else curr->right = add_impl(hobbit, std::move(curr->right), success);

    update_height(curr.get());

    int h_diff = height_diff(curr.get());
    if (h_diff > 1 && height_diff(curr->right.get()) >= 0) {
      curr = left_rotation(std::move(curr));
    } else if (h_diff > 1) {
      curr->right = right_rotation(std::move(curr->right));
      curr = left_rotation(std::move(curr));
    } 

    else if (h_diff < -1 && height_diff(curr->left.get()) <= 0) {
      curr = right_rotation(std::move(curr));
    } else if (h_diff < -1) {
      curr->left = left_rotation(std::move(curr->left));
      curr = right_rotation(std::move(curr));
    }

    return curr;
  }

  std::optional<Hobbit> erase(const std::string& hobbit_name) {
    std::optional<Hobbit> ret = std::nullopt;
    root = erase_impl(hobbit_name, std::move(root), ret);
    return ret;
  }

  std::unique_ptr<Node> erase_impl(const std::string& hobbit_name, std::unique_ptr<Node> curr, std::optional<Hobbit>& ret) {
    if (!curr) return curr;
    push_enchant(curr.get());

    if (curr->hobbit.name > hobbit_name) curr->left = erase_impl(hobbit_name, std::move(curr->left), ret);
    else if (curr->hobbit.name < hobbit_name) curr->right = erase_impl(hobbit_name, std::move(curr->right), ret);
    else { // found hobbit that is to be erased
      ret = curr->hobbit;

      if (!curr->left && !curr->right) return nullptr;
      else if (curr->left && !curr->right) return std::move(curr->left);
      else if (!curr->left && curr->right) return std::move(curr->right);
      else { // 2 children
        Node* succ = curr->right.get();
        push_enchant(succ);
        while (succ->left) {
          succ = succ->left.get();
          push_enchant(succ);
        }

        curr->hobbit = succ->hobbit;
        std::optional<Hobbit> dummy; // so that my ret value is not overwritten
        curr->right = erase_impl(succ->hobbit.name, std::move(curr->right), dummy);
      }
    }

    update_height(curr.get());

    int h_diff = height_diff(curr.get());
    if (h_diff > 1) {
      if (height_diff(curr->right.get()) >= 0) {
        curr = left_rotation(std::move(curr));
      } else {
        curr->right = right_rotation(std::move(curr->right));
        curr = left_rotation(std::move(curr));
      }
    } 

    else if (h_diff < -1) {
      if (height_diff(curr->left.get()) <= 0) {
        curr = right_rotation(std::move(curr));
      } else {
        curr->left = left_rotation(std::move(curr->left));
        curr = right_rotation(std::move(curr));
      }
    }

    return curr;
  }

  std::optional<Hobbit> stats(const std::string& hobbit_name) {
    Node* curr = root.get();
    while (curr) {
      push_enchant(curr);
      if (curr->hobbit.name == hobbit_name) return curr->hobbit;
      curr = hobbit_name < curr->hobbit.name ? curr->left.get() : curr->right.get();
    }
    return std::nullopt;
  }

  bool enchant(
    const std::string& first,
    const std::string& last,
    int hp_diff,
    int off_diff,
    int def_diff
  ) {
    if (first > last) return true;
    if (enchant_possible(root.get(), first, last, hp_diff)) {
      fast_enchant(first, last, {hp_diff, off_diff, def_diff});
      return true;
    }
    return false;
  }

  static void push_enchant(Node* n) {
    if (!n || n->pending.empty()) return;
    if (n->right) n->right->pending.add(n->pending);
    if (n->left) n->left->pending.add(n->pending);
    enchant_hobbit(n, n->pending);
    n->pending.clear();
  }

  void fast_enchant(const std::string& first, const std::string& last, PendingEnchant enchant) {
    Node* curr = root.get();
    while (curr) {
      push_enchant(curr);
      if (curr->hobbit.name < first) {
        curr = curr->right.get();
        continue;
      } else if (curr->hobbit.name > last) {
        curr = curr->left.get();
        continue;
      }

      // curr is now inside the enchant range
      enchant_hobbit(curr, enchant);
      Node* first_applicable = curr->left.get();
      Node* last_applicable = curr->right.get();

      // right border of enchant:
      while (last_applicable) {
        if (last_applicable->hobbit.name <= last) {
          enchant_hobbit(last_applicable,enchant);
          if (last_applicable->left)
            last_applicable->left->pending.add(enchant);
          last_applicable = last_applicable->right.get();
        } else {
          last_applicable = last_applicable->left.get();
        }
      }

      // left border of enchant:
      while (first_applicable) {
        if (first_applicable->hobbit.name >= first) {
          enchant_hobbit(first_applicable,enchant);
          if (first_applicable->right)
            first_applicable->right->pending.add(enchant);
          first_applicable = first_applicable->left.get();
        } else {
          first_applicable = first_applicable->right.get();
        }
      }
      break;
    }
  }

  static void enchant_hobbit(Node* n, PendingEnchant& enchant) {
    n->hobbit.hp += enchant.hp;
    n->hobbit.off += enchant.off;
    n->hobbit.def += enchant.def;
  }


  bool enchant_possible(
    Node* curr,
    const std::string& first,
    const std::string& last,
    int hp_diff
  ) {
    if (!curr) return true;
    push_enchant(curr);

    if (curr->hobbit.name >= first && !enchant_possible(curr->left.get(), first, last, hp_diff)) return false;
    if (curr->hobbit.name >= first && curr->hobbit.name <= last && curr->hobbit.hp + hp_diff <= 0) return false;
    if (curr->hobbit.name <= last && !enchant_possible(curr->right.get(), first, last, hp_diff)) return false;

    return true;
  }

  void slow_enchant(
    Node* curr,
    const std::string& first,
    const std::string& last,
    int hp_diff,
    int off_diff,
    int def_diff
  ) {
    if (!curr) return;

    push_enchant(curr);

    if (curr->hobbit.name >= first)
      slow_enchant(curr->left.get(), first, last, hp_diff, off_diff, def_diff);

    if (curr->hobbit.name >= first && curr->hobbit.name <= last) {
      curr->hobbit.hp += hp_diff;
      curr->hobbit.off += off_diff;
      curr->hobbit.def += def_diff;
    }

    if (curr->hobbit.name <= last)
      slow_enchant(curr->right.get(), first, last, hp_diff, off_diff, def_diff);
  }


  void for_each(auto&& fun) const {
    for_each_impl(root.get(), fun);
  }

  private:
  static void for_each_impl(Node *n, auto& fun) {
    if (!n) return;
    push_enchant(n);
    for_each_impl(n->left.get(), fun);
    fun(n->hobbit);
    for_each_impl(n->right.get(), fun);
  }

  std::unique_ptr<Node> left_rotation(std::unique_ptr<Node> x) {
    push_enchant(x.get());
    push_enchant(x->right.get());

    std::unique_ptr<Node> y = std::move(x->right);
    if (y->left) push_enchant(y->left.get());

    x->right = std::move(y->left);
    update_height(x.get());

    y->left = std::move(x);
    update_height(y.get());
    return y;
  }

  std::unique_ptr<Node> right_rotation(std::unique_ptr<Node> x) {
    push_enchant(x.get());
    push_enchant(x->left.get());

    std::unique_ptr<Node> y = std::move(x->left);
    if (y->right) push_enchant(y->right.get());

    x->left = std::move(y->right);
    update_height(x.get());

    y->right = std::move(x);
    update_height(y.get());
    return y;
  }
  
  int height_diff(Node* n) const {
    if (!n) return 0;
    int l_h = n->left ? n->left->height : 0;
    int r_h = n->right ? n->right->height : 0;
    return r_h - l_h;
  }

  void update_height(Node* n) {
    if (!n) return;
    int l_h = n->left ? n->left->height : 0;
    int r_h = n->right ? n->right->height : 0;
    n->height = std::max(l_h, r_h) + 1;
  }
};

////////////////// ////////////////////////

template < typename T >
auto quote(const T& t) { return t; }

std::string quote(const std::string& s) {
  std::string ret = "\"";
  for (char c : s) if (c != '\n') ret += c; else ret += "\\n";
  return ret + "\"";
}

#define STR_(a) #a
#define STR(a) STR_(a)

#define CHECK_(a, b, a_str, b_str) do { \
    auto _a = (a); \
    decltype(a) _b = (b); \
    if (_a != _b) { \
      std::cout << "Line " << __LINE__ << ": Assertion " \
        << a_str << " == " << b_str << " failed!" \
        << " (lhs: " << quote(_a) << ")" << std::endl; \
      fail++; \
    } else ok++; \
  } while (0)

#define CHECK(a, b) CHECK_(a, b, #a, #b)

 
////////////////// ////////////////////////


void check_army(const HobbitArmy& A, const std::vector<Hobbit>& ref, int& ok, int& fail) {
  size_t i = 0;

  A.for_each([&](const Hobbit& h) {
    CHECK(i < ref.size(), true);
    CHECK(h, ref[i]);
    i++;
  });

  CHECK(i, ref.size());
}

void test1(int& ok, int& fail) {
  HobbitArmy A;
  check_army(A, {}, ok, fail);

  CHECK(A.add({"Frodo", 100, 10, 3}), true);
  CHECK(A.add({"Frodo", 200, 10, 3}), false);
  CHECK(A.erase("Frodo"), std::optional(Hobbit("Frodo", 100, 10, 3)));
  CHECK(A.add({"Frodo", 200, 10, 3}), true);

  CHECK(A.add({"Sam", 80, 10, 4}), true);
  CHECK(A.add({"Pippin", 60, 12, 2}), true);
  CHECK(A.add({"Merry", 60, 15, -3}), true);
  CHECK(A.add({"Smeagol", 0, 100, 100}), false);

  if constexpr(HobbitArmy::CHECK_NEGATIVE_HP)
    CHECK(A.add({"Smeagol", -100, 100, 100}), false);

  CHECK(A.add({"Smeagol", 200, 100, 100}), true);

  CHECK(A.enchant("Frodo", "Frodo", 10, 1, 1), true);
  CHECK(A.enchant("Sam", "Frodo", -1000, 1, 1), true); // empty range
  CHECK(A.enchant("Bilbo", "Bungo", 1000, 0, 0), true); // empty range
  
  if constexpr(HobbitArmy::CHECK_NEGATIVE_HP)
    CHECK(A.enchant("Frodo", "Sam", -60, 1, 1), false);

  CHECK(A.enchant("Frodo", "Sam", 1, 0, 0), true);
  CHECK(A.enchant("Frodo", "Sam", -60, 1, 1), true);

  CHECK(A.stats("Gandalf"), std::optional<Hobbit>{});
  CHECK(A.stats("Frodo"), std::optional(Hobbit("Frodo", 151, 12, 5)));
  CHECK(A.stats("Merry"), std::optional(Hobbit("Merry", 1, 16, -2)));

  check_army(A, {
    {"Frodo", 151, 12, 5},
    {"Merry", 1, 16, -2},
    {"Pippin", 1, 13, 3},
    {"Sam", 21, 11, 5},
    {"Smeagol", 200, 100, 100},
  }, ok, fail);
}

int main() {
  // Incremental tests:
  //HobbitArmy a;
  //a.add({"Frodo", 100, 10, 3});
  //a.add({"Smeagol", 100, 10, 3});
  //a.add({"Bilbo", 100, 10, 3});
  //a.add({"Cilbo", 69, 10, 3});

  ////a.for_each([&](const Hobbit& h) { std::cout << h << std::endl; });
  ////std::cout << "L of ROOT height: " << a.root->left->height << std::endl;
  ////std::cout << "R of L of ROOT height: " << a.root->left->right->height << std::endl;
  ////std::cout << "ROOT height: " << a.root->height << std::endl;
  ////std::cout << "R of ROOT height: " << a.root->right->height << std::endl;
  ////for (int i = 0; i < 26; i++) {
  ////  std::string x = "J";
  ////  x.push_back(char('a' + i));
  ////  a.add({x, 1, 2, 3});
  ////}
  //std::cout << "L of ROOT: " << a.root->left->hobbit << std::endl;
  //std::cout << "ROOT: " << a.root->hobbit << std::endl;
  //std::cout << "R of ROOT: " << a.root->right->hobbit << std::endl;
  //a.for_each([&](const Hobbit& h) { std::cout << h << std::endl; });
  //std::cout << "_______ Erase Smeagol (he's bad) ______\n";

  //a.erase("Smeagol");
  //std::cout << "L of ROOT: " << a.root->left->hobbit << std::endl;
  //std::cout << "ROOT: " << a.root->hobbit << std::endl;
  //std::cout << "R of ROOT: " << a.root->right->hobbit << std::endl;

  int ok = 0, fail = 0;
  test1(ok, fail);

  if (!fail) std::cout << "Passed all " << ok << " tests!" << std::endl;
  else std::cout << "Failed " << fail << " of " << (ok + fail) << " tests." << std::endl;
}


