// by Grace Hill
// tested with Microsoft Visual Studio 2019
// I suspect it works with your GCCs and Clangs, too, but I haven't checked.

#include <unordered_set>
#include <string>
#include <array>
#include <iostream>
#include <fstream>
#include <omp.h>
#include <atomic>

using std::string;
using std::unordered_set;

const unordered_set<string> States = { "AL", "AK", "AS", "AZ", "AR", "CA", "CO", "CT", "DE", "DC", "FM", "FL", "GA", "GU", "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MH", "MD", "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH", "NJ", "NM", "NY", "NC", "ND", "MP", "OH", "OK", "OR", "PW", "PA", "PR", "RI", "SC", "SD", "TN", "TX", "UT", "VT", "VI", "VA", "WA", "WV", "WI", "WY"
	, "AE", "AP", "AA"
}; //pick if you want the three bonus "military" states.

// FM is a pretty rich one to start one, apparently.
// FM NV AL particularly so
const std::array<string, 62> States_Arr = { "FM",  "NV", "AL", "AK", "AS", "AZ", "AR", "CA", "CO", "CT", "DE", "DC",  "FL", "GA", "GU", "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MH", "MD", "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NH", "NJ", "NM", "NY", "NC", "ND", "MP", "OH", "OK", "OR", "PW", "PA", "PR", "RI", "SC", "SD", "TN", "TX", "UT", "VT", "VI", "VA", "WA", "WV", "WI", "WY"
	, "AE", "AP", "AA"
}; //pick if you want the three bonus "military" states.

bool string_good(const string& str)
{
	// mutate one string, don't keep reallocating
	string check = "AA";
	check[0] = 'A';
	check[1] = 'A';
	int start = 0;

	// we only have to check the end of strings, because we know the first chunk is good
	// (if it wasn't good, we wouldn't be here)
	// you can probably get away with doing the first four, but might as well be careful
	// this is not the bottleneck (the sheer number of things to check is!)
	// and since we start off with a string of length six, you gotta check that whole thing first.
	// this makes sure that each adjacent pair of characters is a valid abbreviation
	if (str.size() > 6)
		start = str.size() - 6;
	for (unsigned int i = start; (i + 1) < str.size(); i++)
	{
		check[0] = str[i];
		check[1] = str[i + 1];
		if (States.count(check) == 0)
		{
			// this pair is not a state
			return false;
		}
	}

	// make sure that each pair of characters is a unique abbreviation.
	unordered_set<string> seen;
	for (unsigned int i = 0; (i + 1) < str.size(); i++)
	{
		check[0] = str[i];
		check[1] = str[i + 1];

		auto checkseen = seen.emplace(check);
		if (checkseen.second == false)
		{
			return false; // not unique pairs
		}

	}
	return true;
}

unsigned int longest = 0;

std::ofstream fout("militaryplusone.txt");
void recurse(string& str, unordered_set<string>& used, unordered_set<string>& knowngood)
{
	// when doing the plusone, it's possible to accidentally construct a state name out of singles,
	// which can lead to recursing on the same string more than once
	// and the way to mitigate it is to make a big hashset of strings that is better than doing all that recursion
	// this used to eat up a ton of memory until I started making the starting prefixes bigger

	//string_good is O(n), and knowngood is O(1), but remember that the string is already in the processor cache
	// and looking up in the hash table requires jumping around in memory
	if (string_good(str) && (knowngood.count(str) == 0))
	{
		// I'm pretty sure 34 is the longest string, so print out those if we see them
		if (str.size() > longest || str.size() >= 34)
		{

#pragma omp critical(longest)
			{
				if (str.size() > longest)
				{
					longest = str.size();
				}
				fout << str << ' ' << str.size() << '\n';
				fout.flush();
			}
		}

		// mutate the string in place so we barely have to allocate memory
		size_t oldsize = str.size();
		str.resize(oldsize + 2);
		for (const auto& abbrev : States)
		{
			auto result = used.emplace(abbrev);

			if (result.second)
			{
				str[oldsize] = abbrev[0];
				str[oldsize + 1] = abbrev[1];
				recurse(str, used, knowngood);
				used.erase(result.first);
			}
		}

		str.pop_back();
		for (char c = 'A'; c <= 'Z'; c++)
		{
			str[oldsize] = c;
			recurse(str, used, knowngood);
		}

		// put it back the way it was when we found it
		str.pop_back();

		// mark this one as good. I used to do this up top, but it saves memory to do it down here
		// especially since the only ones who might try to recurse into this string again are
		// further up the call stack- any given string can only recurse into strings longer than it
		knowngood.emplace(str);
	}

}

void start_recurse(const std::string& first, const std::string& second, const std::string& third)
{
	std::string toCheck = first + second + third;
	unordered_set<string> seen{ first, second, third };
	unordered_set<string> knowngood;
	recurse(toCheck, seen, knowngood);
}

int main()
{
	std::atomic<int> progress = 0;
	constexpr int total = States_Arr.size() * States_Arr.size() * States_Arr.size();

	// if you start with strings of length six, you prune the tree much faster
	// this means immense memory savings because you can free the record of 
	// which strings you've already seen sooner, and the strings that could be the longest
	// don't have as much baggage to carry around.


	// use the /openmp or -fopenmp switch on your compiler, if you have one
	// this works sequentially, too, but much slower

#pragma omp parallel for collapse(3)
	for (int i = 0; i < States_Arr.size(); i++)
	{
		for (int j = 0; j < States_Arr.size(); j++)
		{
			for (int k = 0; k < States_Arr.size(); k++)
			{

				if (i == j || i == k || k == j)
				{
					++progress;
					continue;
				}

#pragma omp critical(cout)
				{
					std::cout << "Starting " << States_Arr[i] << States_Arr[j] << States_Arr[k] << '\n';
				}

				start_recurse(States_Arr[i], States_Arr[j], States_Arr[k]);

#pragma omp critical(cout)
				{
					std::cout << "Finished with " << States_Arr[i] << States_Arr[j] << States_Arr[k] << " (" << ++progress << " out of " << total << ")\n";
				}

			}
		}
	}

	std::cout << "Finished!\n";
}
