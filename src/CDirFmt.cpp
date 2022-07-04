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
              "-nocolor:f   (ignore colors) "
              "-color:f     (set terminal color) "
              "-clip:s      (clip left, right, middle of all of directory) "
              "-dir:s       (directory to format)");

  cargs.parse(&argc, argv);

  if (cargs.isHelp())
    exit(1);

  bool        split     = ! cargs.getBooleanArg("-nosplit") && cargs.getBooleanArg("-split");
  bool        prompt    = cargs.getBooleanArg("-prompt");
  std::string postfix   = cargs.getStringArg ("-postfix");
  int         break_len = int(cargs.getIntegerArg("-break_len"));
  bool        color     = cargs.getBooleanArg("-color");
  bool        nocolor   = cargs.getBooleanArg("-nocolor");
  std::string clip      = cargs.getStringArg ("-clip");
  std::string dir       = cargs.getStringArg ("-dir");

  if (! cargs.isBooleanArgSet("-clip") && getenv("DIRFMT_CLIP"))
    clip = getenv("DIRFMT_CLIP");

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
  dirfmt.setNoColor(nocolor);
  dirfmt.setColor(color);
  dirfmt.setClip(clip);

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

  envLen_ = uint(env_.size());

  parseEnv();

  //---

  char *colorEnv = getenv("DIRFMT_PROMPT_COLOR");

  if (colorEnv)
    promptColor_ = colorEnv;

  //---

  char *postfixEnv = getenv("DIRFMT_POSTFIX_COLOR");

  if (postfixEnv) {
    uint i      = 0;
    uint envLen = uint(strlen(postfixEnv));

    std::string name;

    while (i < envLen) {
      // handle named color
      if (postfixEnv[i] == '=') {
        ++i;

        std::string value;

        while (i < envLen && postfixEnv[i] != ',')
          value += postfixEnv[i++];

        if (i < envLen && postfixEnv[i] == ',')
          ++i;

        if      (name == "bg")
          postfixBgColor_ = value;
        else if (name == "fg")
          postfixFgColor_ = value;

        name = "";
      }
      else {
        name += postfixEnv[i++];
      }
    }

    if (name != "" && postfixFgColor_ == "")
      postfixFgColor_ = name;
  }

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
  uint i = 0;

  while (i < envLen_) {
    // skip space
    while (i < envLen_ && isspace(env_[i]))
      ++i;

    //---

    // find text before '=' and next space
    uint j = i;

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

  uint len = uint(directory.size());

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

  len = uint(directory.size());

  //---

  std::string promptColorStr;

  if (! isNoColor())
    promptColorStr = colorEscape(promptColor_, prompt_);

  std::string normColorStr;

  if (! isNoColor())
    normColorStr = colorEscape("norm", prompt_);

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

    std::string colorStr;

    if (! isNoColor()) {
      colorStr += colorEscape(format.bg, prompt_, /*bg*/true);
      colorStr += colorEscape(format.fg, prompt_, /*bg*/false);
    }

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

  uint len1 = 0;

  for (const auto &part : parts) {
    if (part.color != "")
      std::cout << part.color;

    uint len2 = len1 + uint(part.str.size());

    // if new string exceeds break length then elide or truncate
    if (breakLen() > 0 && len2 >= uint(breakLen())) {
      // split onto new line
      if (split_) {
        newline = true;

        std::cout << part.str;
      }
      else {
        elide = true;

        uint clip_len = uint(breakLen()) - len1 - 3;

        if (clip() == "left") {
          std::cout << "...";
          std::cout << part.str.substr(part.str.size() - clip_len, clip_len);
        }
        else {
          std::cout << part.str.substr(0, clip_len);
          std::cout << "...";
        }
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
  if (breakLen() > 0 && len > uint(breakLen()) + 3) {
    // split onto new line
    if (split_) {
      std::cout << directory;
      std::cout << "\012\013";
    }
    else {
      // TODO: don't split in color
      uint len1 = (uint(breakLen()) + 3)/2;
      uint len2 = breakLen() + 3 - len1;

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

  if (postfix_ != "") {
    std::string postfixColorBgStr, postfixColorFgStr;

    if (! isNoColor()) {
      postfixColorBgStr = colorEscape(postfixBgColor_, prompt_, /*bg*/true);
      postfixColorFgStr = colorEscape(postfixFgColor_, prompt_, /*bg*/false);
    }

    std::cout << postfixColorBgStr << postfixColorFgStr << postfix_ << normColorStr;
  }
}

std::string
CDirFmt::
colorEscape(const std::string &str, bool prompt, bool bg) const
{
  std::string colorStr = str;

  //---

  bool bold = (colorStr == "bold");
  bool norm = (colorStr == "norm");

  //---

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

  if      (term_ == "iris-ansi" || term_ == "iris-ansi-net") {
    if      (norm)
      ret += "[0m";
    else if (bold)
      ret += "[1m";
  }
  else if (term_ == "hpterm") {
    if      (norm)
      ret += "&v0S";
    else if (bold)
      ret += "&v3S";
  }
  else if (term_ == "xterm" || term_ == "xterm-color" || term_ == "xterm-256color") {
    if      (bold)
      ret += "[1m";
    else if (norm)
      ret += "[0m";

    if (colorTerm_) {
      if (alt) {
        if (bg) {
          if      (colorStr == "black"  ) ret += "[100m";
          else if (colorStr == "red"    ) ret += "[101m";
          else if (colorStr == "green"  ) ret += "[102m";
          else if (colorStr == "yellow" ) ret += "[103m";
          else if (colorStr == "blue"   ) ret += "[104m";
          else if (colorStr == "magenta") ret += "[105m";
          else if (colorStr == "cyan"   ) ret += "[106m";
          else if (colorStr == "white"  ) ret += "[107m";
        }
        else {
          if      (colorStr == "black"  ) ret += "[90m";
          else if (colorStr == "red"    ) ret += "[91m";
          else if (colorStr == "green"  ) ret += "[92m";
          else if (colorStr == "yellow" ) ret += "[93m";
          else if (colorStr == "blue"   ) ret += "[94m";
          else if (colorStr == "magenta") ret += "[95m";
          else if (colorStr == "cyan"   ) ret += "[96m";
          else if (colorStr == "white"  ) ret += "[97m";
        }
      }
      else {
        if (bg) {
          if      (colorStr == "black"  ) ret += "[40m";
          else if (colorStr == "red"    ) ret += "[41m";
          else if (colorStr == "green"  ) ret += "[42m";
          else if (colorStr == "yellow" ) ret += "[43m";
          else if (colorStr == "blue"   ) ret += "[44m";
          else if (colorStr == "magenta") ret += "[45m";
          else if (colorStr == "cyan"   ) ret += "[46m";
          else if (colorStr == "white"  ) ret += "[47m";
        }
        else {
          if      (colorStr == "black"  ) ret += "[30m";
          else if (colorStr == "red"    ) ret += "[31m";
          else if (colorStr == "green"  ) ret += "[32m";
          else if (colorStr == "yellow" ) ret += "[33m";
          else if (colorStr == "blue"   ) ret += "[34m";
          else if (colorStr == "magenta") ret += "[35m";
          else if (colorStr == "cyan"   ) ret += "[36m";
          else if (colorStr == "white"  ) ret += "[37m";
        }
      }
    }
  }

  if (ret == "")
    return "";

  if (prompt && isTcsh())
    ret = escapeTcsh(ret);

  return ret;
}

std::string
CDirFmt::
fillEscape(const std::string &str, bool prompt) const
{
  if (str == "")
    return "";

  std::string ret;

  if (prompt) {
    if (isTcsh()) {
      //return escapeTcsh("]11;" + str + "\\");
      return "";
    }
  }

  if      (term_ == "iris-ansi" || term_ == "iris-ansi-net") {
  }
  else if (term_ == "hpterm") {
  }
  else if (term_ == "xterm" || term_ == "xterm-color" || term_ == "xterm-256color") {
    return "]11;" + str + "\\";
  }

  return "";
}

bool
CDirFmt::
isTcsh() const
{
  auto p = shellName_.find("tcsh");

  return (p != std::string::npos);
}

std::string
CDirFmt::
escapeTcsh(const std::string &str) const
{
  return "%{" + str + "%}";
}
