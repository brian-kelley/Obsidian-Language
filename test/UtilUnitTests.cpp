#include "Utils.hpp"

namespace Base64Testing
{
  int runBase64ED(uint64_t val)
  {
    string s = base64Encode(val);
    uint64_t out = base64Decode(s);
    if(val != out)
    {
      cout << "Value " << val << " encoded is " << s << ",\n";
      cout << "but decoding that produced " << out << '\n';
      return 1;
    }
    //otherwise, success
    return 0;
  }

  int runBase64DE(string val)
  {
    uint64_t u = base64Decode(val);
    string out = base64Encode(u);
    if(val != out)
    {
      cout << "String " << val << " decoded is " << u << ",\n";
      cout << "but encoding that produced " << out << '\n';
      return 1;
    }
    //otherwise, success
    return 0;
  }

  int test()
  {
    //Make sure both encode*decode and decode*encode are no-ops
    int failures = 0;
    failures += runBase64ED(0);
    failures += runBase64ED(1);
    failures += runBase64ED(5);
    failures += runBase64ED(12345);
    failures += runBase64ED(0xFFFFFFFF);
    failures += runBase64ED(0xFFFFFFFFFFFFFFFFULL);
    failures += runBase64ED(0xABCDABCDABCDABCDULL);
    //Try some random encoded strings
    failures += runBase64DE("");
    failures += runBase64DE("1");
    failures += runBase64DE("+");
    failures += runBase64DE("/");
    failures += runBase64DE("Abcd/");
    failures += runBase64DE("Brian/");
    failures += runBase64DE("BrIaN+/+");
    failures += runBase64DE("FFFFFFFF");
    failures += runBase64DE("abcdefg");
    failures += runBase64DE("hijklmno");
    failures += runBase64DE("uvwxyz");
    failures += runBase64DE("012345");
    failures += runBase64DE("6a7b8c9d");
    return 0;
  }
}

int main()
{
  int failures = 0;
  failures += Base64Testing::test();
  return failures;
}
