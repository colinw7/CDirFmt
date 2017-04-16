#include <CDirFmt.h>
#include <CArgs.h>

int
main(int argc, char **argv)
{
  CArgs cargs("-nosplit:f   (split long lines) "
              "-split:f     (don't split long lines) "
              "-prompt:f    (output is for prompt) "
              "-postfix:s   (display postfix chars) "
              "-break_len:i (break length) "
              "-color:f     (set terminal color) "
              "-dir:s       (directory to format)");

  cargs.parse(&argc, argv);

  if (cargs.isHelp())
    exit(1);

  bool        split     = ! cargs.getBooleanArg("-nosplit") && cargs.getBooleanArg("-split");
  bool        prompt    = cargs.getBooleanArg("-prompt");
  std::string postfix   = cargs.getStringArg("-postfix");
  int         break_len = cargs.getIntegerArg("-break_len");
  bool        color     = cargs.getBooleanArg("-color");
  std::string dir       = cargs.getStringArg("-dir");

  if (break_len <= 0)
    break_len = CDirFmt::BREAK_LEN;

  //------

  // get directory name from input
  std::string directory;

  if (dir == "") {
    int c;

    while ((c = fgetc(stdin)) != EOF)
      directory += char(c);
  }
  else
    directory = dir;

  //---

  // set up formatter
  CDirFmt dirfmt;

  dirfmt.setSplit(split);
  dirfmt.setPrompt(prompt);
  dirfmt.setPostfix(postfix);
  dirfmt.setBreakLen(break_len);
  dirfmt.setColor(color);

  // format
  dirfmt.format(directory);

  exit(0);
}

CDirFmt::
CDirFmt()
{
  // get format definition
  const char *dirFmtEnv = getenv("DIRFMT");

  if (dirFmtEnv)
    env_ = dirFmtEnv;

  envLen_ = env_.size();

  parseEnv();

  //---

  char *colorEnv = getenv("DIRFMT_PROMPT_COLOR");

  if (colorEnv)
    promptColor_ = colorEnv;

  //---

  // get terminal type (for output)
  const char *termEnv = getenv("TERM");

  if (termEnv)
    term_ = termEnv;
  else
    term_ = "xterm";

  //---

  // get shell type (for prompt)
  const char *shellEnv = getenv("SHELL");

  if (shellEnv)
    shellName_ = shellEnv;
  else
    shellName_ = "/bin/sh";
}

void
CDirFmt::
parseEnv()
{
  // parse format definition
  int i = 0;

  while (i < envLen_) {
    // skip space
    while (i < envLen_ && isspace(env_[i]))
      ++i;

    //---

    // find text before '=' and next space
    int j = i;

    std::string name, value;
    std::string bg, fg, fill;

    while (i < envLen_) {
      // skip escaped char
      if      (env_[i] == '\\' && env_[i + 1] != '\0') {
        ++i;
      }
      // handle color followed by assign
      else if (env_[i] == '@') {
        name = env_.substr(j, i - j);

        while (env_[i] == '@') {
          ++i;

          std::string arg;

          while (i < envLen_ && ! isspace(env_[i]) && env_[i] != '=' && env_[i] != '@')
            arg += env_[i++];

          std::string name1, value1;

          uint k = 0;

          while (k < arg.size()) {
            if (arg[k] == ':') {
              name1  = arg.substr(0, k);
              value1 = arg.substr(k + 1);
              break;
            }

            ++k;
          }

          if (name1 == "") {
            name1  = "fg";
            value1 = arg;
          }

          if      (name1 == "fg")
            fg   = value1;
          else if (name1 == "bg")
            bg   = value1;
          else if (name1 == "fill")
            fill = value1;
        }

        //---

        if (env_[i] == '=') {
          ++i;

          while (i < envLen_ && ! isspace(env_[i]))
            value += env_[i++];
        }

        while (i < envLen_ && isspace(env_[i]))
          ++i;

        break;
      }
      // handle simple assign
      else if (env_[i] == '=') {
        name = env_.substr(j, i - j);

        ++i;

        while (i < envLen_ && ! isspace(env_[i]))
          value += env_[i++];

        while (i < envLen_ && isspace(env_[i]))
          ++i;

        break;
      }
      // handle space (no assign)
      else if (isspace(env_[i])) {
        name = env_.substr(j, i - j);

        while (i < envLen_ && isspace(env_[i]))
          ++i;

        break;
      }
      else {
        ++i;
      }
    }

    // add format definition (name=value and color)
    addFormat(name, value, bg, fg, fill);
  }
}

void
CDirFmt::
format(const std::string &dir) const
{
  // trim directory to max len
  std::string directory = dir;

  int len = directory.size();

  if (len > MAX_DIR) {
    directory = directory.substr(0, MAX_DIR);
    len       = MAX_DIR;
  }

  //---

  // remove trailing spaces
  if (len > 0 && isspace(directory[len - 1])) {
    while (len > 0 && isspace(directory[len - 1]))
      --len;

    directory = directory.substr(0, len);
  }

  len = directory.size();

  //---

  std::string promptColorStr = colorEscape(promptColor_, prompt_);

  std::string normColorStr = colorEscape("norm", prompt_);

  //---

  std::string fillStr;

  typedef std::vector<Part> Parts;

  Parts parts;

  for (const auto &format : formats_) {
    // check if format name in directory
    auto p = directory.find(format.name);

    if (p == std::string::npos)
      continue;

    parts.emplace_back(directory.substr(0, p), promptColorStr);

    directory = directory.substr(p + format.name.size());

    //---

    std::string colorStr = colorEscape(format.fg, prompt_);

    fillStr = fillEscape(format.fill, prompt_);

    parts.emplace_back(format.value, colorStr);
  }

  parts.emplace_back(directory, promptColorStr);

  //---

  if (color_) {
    std::cout << fillStr;
    return;
  }

  //---

  bool elide   = false;
  bool newline = false;

  int len1 = 0;

  for (const auto &part : parts) {
    if (part.color != "")
      std::cout << part.color;

    int len2 = len1 + part.str.size();

    // if new string exceeds break length then elide or truncate
    if (len2 >= breakLen_) {
      // split onto new line
      if (split_) {
        newline = true;

        std::cout << part.str;
      }
      else {
        elide = true;

        std::cout << part.str.substr(0, breakLen_ - len1 - 3);

        std::cout << "...";
      }
    }
    else
      std::cout << part.str;

    if (part.color != "")
      std::cout << normColorStr;

    len1 = len2;

    if (elide)
      break;
  }

  if (newline)
    std::cout << "\012\013";

#if 0
  if (len > breakLen_ + 3) {
    // split onto new line
    if (split_) {
      std::cout << directory;
      std::cout << "\012\013";
    }
    else {
      // TODO: don't split in color
      int len1 = (breakLen_ + 3)/2;
      int len2 = breakLen_ + 3 - len1;

      std::string str1 = directory.substr(0, len1);
      std::string str2 = directory.substr(len - len2);

      std::cout << str1 << "..." << str2;
    }
  }
  else {
    std::cout << directory;
  }
#endif

  //---

  // output prompt

  if (postfix_ != "")
    std::cout << postfix_;
}

std::string
CDirFmt::
colorEscape(const std::string &str, bool prompt) const
{
  std::string colorStr = str;

  bool bold = false;

  auto p = colorStr.find("bold-");

  if (p == 0) {
    bold = true;

    colorStr = colorStr.substr(p + 5);
  }

  //---

  bool alt = false;

  p = colorStr.find("alt-");

  if (p == 0) {
    alt = true;

    colorStr = colorStr.substr(p + 4);
  }

  //---

  std::string ret;

  // 3 fg, 4 bg
  if (prompt) {
    if (shellName_ == "/bin/tcsh") {
      if (bold)
        ret += "%{[1m%}";
      else
        ret += "%{[0m%}";

      if (alt) {
        if (colorStr == "black"  ) ret += "%{[90m%}";
        if (colorStr == "red"    ) ret += "%{[91m%}";
        if (colorStr == "green"  ) ret += "%{[92m%}";
        if (colorStr == "yellow" ) ret += "%{[93m%}";
        if (colorStr == "blue"   ) ret += "%{[94m%}";
        if (colorStr == "magenta") ret += "%{[95m%}";
        if (colorStr == "cyan"   ) ret += "%{[96m%}";
        if (colorStr == "white"  ) ret += "%{[97m%}";
      }
      else {
        if (colorStr == "black"  ) ret += "%{[30m%}";
        if (colorStr == "red"    ) ret += "%{[31m%}";
        if (colorStr == "green"  ) ret += "%{[32m%}";
        if (colorStr == "yellow" ) ret += "%{[33m%}";
        if (colorStr == "blue"   ) ret += "%{[34m%}";
        if (colorStr == "magenta") ret += "%{[35m%}";
        if (colorStr == "cyan"   ) ret += "%{[36m%}";
        if (colorStr == "white"  ) ret += "%{[37m%}";
      }

      return ret;
    }
  }

  if      (term_ == "iris-ansi" || term_ == "iris-ansi-net") {
    if (colorStr == "norm") return "[0m";
    if (colorStr == "bold") return "[1m";
  }
  else if (term_ == "hpterm") {
    if (colorStr == "norm") return "&v0S";
    if (colorStr == "bold") return "&v3S";
  }
  else if (term_ == "xterm") {
    if (colorStr == "norm") return "[0m";
    if (colorStr == "bold") return "[1m";

    if (colorTerm_) {
      if (colorStr == "black"  ) return "[30m";
      if (colorStr == "red"    ) return "[31m";
      if (colorStr == "green"  ) return "[32m";
      if (colorStr == "yellow" ) return "[33m";
      if (colorStr == "blue"   ) return "[34m";
      if (colorStr == "magenta") return "[35m";
      if (colorStr == "cyan"   ) return "[36m";
      if (colorStr == "white"  ) return "[37m";
    }
  }

  return "";
}

std::string
CDirFmt::
fillEscape(const std::string &str, bool prompt) const
{
  if (str == "")
    return "";

  std::string ret;

  if (prompt) {
    if (shellName_ == "/bin/tcsh") {
      //return "%{]11;" + str + "\\%}";
      return "";
    }
  }

  if      (term_ == "iris-ansi" || term_ == "iris-ansi-net") {
  }
  else if (term_ == "hpterm") {
  }
  else if (term_ == "xterm") {
    return "]11;" + str + "\\";
  }

  return "";
}
