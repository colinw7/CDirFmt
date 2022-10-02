#ifndef CDirFmt_H
#define CDirFmt_H

#include <string>
#include <vector>

class CDirFmt {
 public:
  enum { BREAK_LEN = 54 };
  enum { MAX_DIR   = 1024 };

 public:
  struct Format {
    Format(const std::string &name_, const std::string &value_,
           const std::string &bg_="", const std::string &fg_="",
           const std::string &fill_="") :
     name(name_), value(value_), bg(bg_), fg(fg_), fill(fill_) {
    }

    std::string name;
    std::string value;
    std::string bg;
    std::string fg;
    std::string fill;
  };

  struct Part {
    std::string str;
    std::string color;

    Part(const std::string &str_, const std::string &color_) :
     str(str_), color(color_) {
    }
  };

 public:
  CDirFmt();

  bool isSplit() const { return split_; }
  void setSplit(bool b) { split_ = b; }

  bool isPrompt() const { return prompt_; }
  void setPrompt(bool b) { prompt_ = b; }

  const std::string &postfix() const { return postfix_; }
  void setPostfix(const std::string &s) { postfix_ = s; }

  int breakLen() const { return breakLen_; }
  void setBreakLen(int i) { breakLen_ = i; }

  bool isNoColor() const { return nocolor_; }
  void setNoColor(bool b) { nocolor_ = b; }

  bool isColor() const { return color_; }
  void setColor(bool b) { color_ = b; }

  const std::string &clip() const { return clip_; }
  void setClip(const std::string &s) { clip_ = s; }

  void format(const std::string &dir) const;

 private:
  void parseEnv();

  void addFormat(const std::string &name, const std::string &value,
                 const std::string &bg="", const std::string &fg="",
                 const std::string &fill="") {
    formats_.emplace_back(name, value, bg, fg, fill);
  }

  std::string colorEscape(const std::string &colorStr, bool prompt, bool bg=false) const;

  std::string fillEscape(const std::string &colorStr, bool prompt) const;

  bool isTcsh() const;

  std::string escapeTcsh(const std::string &str) const;

 private:
  typedef std::vector<Format> Formats;

  bool        split_     { false };
  bool        prompt_    { false };
  std::string postfix_;
  int         breakLen_  { -1 };
  bool        nocolor_   { false };
  bool        color_     { false };
  std::string env_;
  uint        envLen_;
  std::string term_;
  bool        colorTerm_ { true };
  std::string clip_      { "right" };
  std::string shellName_;
  std::string promptColor_;
  std::string postfixBgColor_;
  std::string postfixFgColor_;
  Formats     formats_;
};

#endif
