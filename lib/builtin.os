//stoi: parse signed decimal integer)
func long? stoi(char[] str)
{
  if(str.len == 0)
  {
    return error;
  }
  bool neg = false;
  long val = 0;
  for [i, it] : str
  {
    if(i == 0 && it == '-')
    {
      neg = true;
    }
    if(it < '0' && it > '9')
    {
      return error;
    }
    val *= 10;
    val += (it - '0');
  }
  if(neg) return -val;
  return val;
}

//stod: parse decimal floating-point (either normal or scientific)
func double? stod(char[] str)
{
  if(str.len == 0)
  {
    return error;
  }
  double val = 0;
  bool sign = false;
  bool pastDot = false;
  bool pastExponent = false;
  //current multiplier for digits right of decimal point
  double fracMult = 0.1;
  int expo = 0;
  bool expoSign = false;
  bool valid = false;
  for [i, it] : str
  {
    bool isDigit = it >= '0' && it <= '9';
    if(i == 0 && it == '-')
    {
      sign = true;
    }
    else if(!pastDot && isDigit)
    {
      //eat one digit for the integer part
      val *= 10;
      val += it - '0';
      //now actually have a value
      valid = true;
    }
    else if(!pastDot && it == '.')
    {
      pastDot = true;
    }
    else if(pastDot && !pastExponent && isDigit)
    {
      //eat one digit for fractional part
      val += (it - '0') * fracMult;
      fracMult /= 10;
    }
    else if(it == 'e' || it == 'E')
    {
      pastExponent = true;
    }
    else if(pastExponent && !expoSign && it == '-')
    {
      expoSign = true;
    }
    else if(pastExponent && isDigit)
    {
      expo *= 10;
      expo += it - '0';
    }
    else
    {
      //unexpected character
      return error;
    }
  }
  if(!valid)
  {
    return error;
  }
  //apply exponent, if it exists
  if(expoSign)
  {
    expo = -expo;
  }
  val *= pow(10, expo);
  if(sign)
  {
    val = -val;
  }
  return val;
}

//unsigned integer to string (hexadecimal)
func char[] printHex(ulong num)
{
  string val;
  while(num > 0)
  {
    int digit = num & 0xF;
    num >>= 4;
    if(digit < 10)
    {
      val = (digit + '0') + val;
    }
    else
    {
      val = (digit - 10 + 'A') + val;
    }
  }
  return "0x" + val;
}

//unsigned integer to string (binary)
func char[] printBin(ulong num)
{
  string val;
  while(num > 0)
  {
    int digit = num & 1;
    num >>= 1;
    val = (digit + '0') + val;
  }
  return "0b" + val;
}

//POSIX File I/O
extern proc int? open(string pathname, uint flags)
"
  int handle = open($0->data, $1);
  if(handle < 0)
    convert$typer_int(&$r, &handle);
  else
    convert$typer_Error(&$r, NULL);
";

extern proc void? close(int handle)
"
  int result = close($0->data);
  if(result == 0)
    convert$typer_void(&$r, NULL);
  else
    convert$typer_Error(&$r, NULL);
";

//note: Onyx read() fails if allocating the read buffer fails
extern proc ubyte[]? read(int handle, ulong bytes)
"
  void* buf = malloc(bytes);
  if(!buf)
    convert$typer_Error(&$r, NULL);
  ulong bytesRead = read(handle, buf, bytes);
  if(bytesRead == bytes)
  {
    //read all bytes successfully

    $r->data = buf
  }
";

