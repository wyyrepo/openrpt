/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2014 by OpenMFG, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */

#include "metasqlqueryparser.h"

#include "regex/regex.h"

#include <algorithm>
#include <string>
#include <vector>

static const char * __wordchars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

// convert a string to a number
static inline double convertToDouble(const std::string & s)
{
    std::istringstream i(s);
    double x;
    if (!(i >> x))
      return 0.0;
    return x;
} 

// lower case a string
static inline std::string strlower(const std::string &cs) {
    std::string s = cs;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// trim from start
static inline std::string ltrim(const std::string &cs) {
    std::string s = cs;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(isspace))));
    return s;
}

// trim from end
static inline std::string rtrim(const std::string &cs) {
    std::string s = cs;
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string trim(const std::string &s) {
    return ltrim(rtrim(s));
}

class MetaSQLOutput {
    public:
        MetaSQLOutput(MetaSQLQueryParser * parent) { _parent = parent; }
        virtual ~MetaSQLOutput() { _parent = 0; }

        virtual std::string toString(MetaSQLInfo *, int * = 0, bool * = 0) = 0;

    protected:
        MetaSQLQueryParser * _parent;
};

class MetaSQLString : public MetaSQLOutput {
    public:
        MetaSQLString(MetaSQLQueryParser * parent, const std::string & str) : MetaSQLOutput(parent), _string(str) {}

        virtual std::string toString(MetaSQLInfo *, int * = 0, bool * = 0) { return _string; }

    protected:
        std::string _string;
};

class MetaSQLComment : public MetaSQLOutput {
    public:
        MetaSQLComment(MetaSQLQueryParser * parent, const std::string & str) : MetaSQLOutput(parent), _string(str) {}

        // If we want to show comments we need to escape single quotes as they cause problems when passed to database server
        // But we don't have to include comments at all since they are not required by the database to work.
        virtual std::string toString(MetaSQLInfo *, int * = 0, bool * = 0) { return " "; }

    protected:
        std::string _string;
};

class MetaSQLFunction : public MetaSQLOutput {
    public:
        MetaSQLFunction(MetaSQLQueryParser * parent, const std::string & func, const std::vector<std::string> & params)
          : MetaSQLOutput(parent) {
            _valid = 0;
            _nBreaks = 0;
            _noOutput = 0;

            _params = params;

            _func = identifyFunction(func);
            if(_func != FunctionUnknown) {
                switch(_func) {
                    case FunctionValue:
                    case FunctionLiteral:
                    case FunctionExists:
                    case FunctionReExists:
                    case FunctionIsFirst:
                    case FunctionIsLast:
                        _valid = (_params.size() >= 1);
                        break;
                    case FunctionContinue:
                    case FunctionBreak:
                        _valid = 1;
                        _noOutput = 1;
                        if(params.size() >= 1)
                            _nBreaks = (int) convertToDouble(params[0]);
                        if(_nBreaks < 1) _nBreaks = 1;
                        break;
                    default:
                        _parent->_logger << "MetaSQLFunction::MetaSQLFunction() encountered unknown Function Type " << (int)_func << "!" << std::endl;
                };
            }
        }

        enum Function {
            FunctionUnknown = 0,
            FunctionValue,
            FunctionLiteral,
            FunctionExists,
            FunctionReExists,
            FunctionIsFirst,
            FunctionIsLast,
            FunctionContinue,
            FunctionBreak
        };

        bool isValid() { return _valid; }
        Function type() { return _func; }

        virtual std::string toString(MetaSQLInfo * mif, int * nBreaks = 0, bool * isContinue = 0) {
            if(_noOutput)
                return "";
            std::string v = toVariant(mif, nBreaks, isContinue);
            return v;
        }
        virtual std::string toVariant(MetaSQLInfo * mif, int * nBreaks = 0, bool * isContinue = 0) {
            std::string val;
            if(_valid) {
                std::string str;
                std::list<std::string> list;
                std::list<std::string>::iterator strlit;
                regex_t re;
                switch(_func) {
                    case FunctionValue:
                    case FunctionLiteral:
                        str = _params[0];
                        val = mif->getValue(str, (_func==FunctionValue));
                        break;
                    case FunctionExists:
                        list = mif->enumerateNames();
                        strlit = find(list.begin(), list.end(), _params[0]);
                        val = ( strlit != list.end() ? mif->trueValue() : mif->falseValue() );
                        break;
                    case FunctionReExists:
                        if(regcomp(&re, _params[0].c_str(), REG_EXTENDED|REG_NOSUB) == 0) {
                            list = mif->enumerateNames();
                            for(strlit = list.begin(); strlit != list.end(); strlit++) {
                                if(regexec(&re, (*strlit).c_str(), (std::size_t)0, NULL, 0) == 0) {
                                    val = mif->trueValue();
                                    break;
                                }
                            }
                            regfree(&re);
                        }
                        break;
                    case FunctionIsFirst:
                        val = (mif->isValueFirst(_params[0]) ? mif->trueValue() : mif->falseValue());
                        break;
                    case FunctionIsLast:
                        val = (mif->isValueLast(_params[0]) ? mif->trueValue() : mif->falseValue());
                        break;
                    case FunctionContinue:
                    case FunctionBreak:
                        if(nBreaks && isContinue) {
                            *nBreaks = _nBreaks;
                            *isContinue = (_func == FunctionContinue);
                        }
                        break;
                    default:
                        _parent->_logger << "MetaSQLFunction::toVariant() encountered unknown Function Type " << (int)_func << "!" << std::endl; 
                        // how did we get here?
                };
            }
            return val;
        }

    protected:
        Function identifyFunction(const std::string & func) {
            std::string f = trim(func);
            if(f == "value")
                return FunctionValue;
            else if(f == "literal")
                return FunctionLiteral;
            else if(f == "exists")
                return FunctionExists;
            else if(f == "reexists")
                return FunctionReExists;
            else if(f == "isfirst")
                return FunctionIsFirst;
            else if(f == "islast")
                return FunctionIsLast;
            else if(f == "continue")
                return FunctionContinue;
            else if(f == "break")
                return FunctionBreak;

            _parent->_logger << "Unable to identify function '" << f << "'!" << std::endl;

            return FunctionUnknown;
        }

    private:
        bool _valid;
        bool _noOutput;
        Function _func;
        std::vector<std::string> _params;
        int _nBreaks;
};

class MetaSQLBlock : public MetaSQLOutput {
    public:
        MetaSQLBlock(MetaSQLQueryParser * parent, const std::string & pCmd, const std::string & pOptions)
          : MetaSQLOutput(parent) {
            _valid = false;

            _alt = 0;
            _if_not = false;
            _if_func = 0;

            _block = identifyBlock(pCmd);
            if(BlockGeneric == _block || BlockElse == _block) {
                _valid = true;
            } else if(BlockIf == _block || BlockElseIf == _block) {
                // hmmm the hard part ;)
                // short solution to just get it to work.
                // there is only one option and that is a single
                // function call that returns true or false.
                // with an optional NOT clause.
                std::string wip = trim(pOptions);
                if(strlower(wip.substr(0,4)) == "not ") {
                    _if_not = true;
                    wip = wip.substr(4);
                }

                std::vector<std::string> plist;
                std::string options;
                std::string cmd;
                std::size_t i = wip.find_first_not_of(__wordchars);
                if(i == std::string::npos) {
                    cmd = wip;
                    options.clear();
                } else {
                    cmd = wip.substr(0,i);
                    options = wip.substr(i);
                }
                cmd = strlower(cmd);
                options = trim(options);

                if(!options.empty()) {
                    // first if we have a '(' then we will only parse out the information between it
                    // and the following ')'
                    char qc = options[0];
                    bool enclosed = false;
                    bool in_string = false;
                    char string_starter = '"';
                    wip.clear();
                    if(qc == '(') enclosed = true;
                    bool working = !enclosed;
                    for(std::size_t p = 0; p < options.size(); p++) {
                        qc = options.at(p);
                        if(!working && enclosed && qc == '(') working = true;
                        else {
                            if(in_string) {
                                if(qc == '\\') {
                                    wip += options.at(++p);
                                } else if(qc == string_starter) {
                                    in_string = false;
                                } else {
                                    wip += qc;
                                }
                            } else {
                                if(qc == ',') {
                                    plist.push_back(wip);
                                    wip.clear();
                                } else if(isspace(qc)) {
                                    // eat white space
                                } else if(qc == '\'' || qc == '"') {
                                    in_string = true;
                                    string_starter = qc;
                                } else if(enclosed && qc == ')') {
                                    working = false;
                                    break;
                                } else {
                                    wip += qc;
                                }
                            }
                        }
                    }
                    if(!wip.empty()) plist.push_back(wip);
                }

                _if_func = new MetaSQLFunction(_parent, cmd, plist);
                if(!_if_func->isValid()) {
                    _parent->_logger << "Failed to create new " << cmd << " function in if/elseif." << std::endl;
                    delete _if_func;
                    _if_func = 0;
                } else {
                    _valid = true;
                }
            } else if(BlockForEach == _block) {
                std::string tmp = trim(pOptions);
                std::string wip;
                bool in_string = false;
                int in_list = 0;
                char string_starter = '"';
                for(std::size_t p = 0; p < tmp.size(); p++) {
                    char qc = tmp.at(p);
                    if(in_string) {
                        if(qc == '\\') wip += tmp.at(++p);
                        else if(qc == string_starter) in_string = false;
                        else wip += qc;
                    } else {
                        if(qc == '(') in_list++;
                        else if(qc == ')') {
                            in_list--;
                            if(in_list < 1) break;
                        } else if(qc == '\'' || qc == '"') {
                            in_string = true;
                            string_starter = qc;
                        } else if(qc == ',') break;
                        // everything else just... disapears?
                    }
                }
                if(!wip.empty()) {
                    _loopVar = wip;
                    _valid = true;
                }
            } else {
                _parent->_logger << "MetaSQLBlock::MetaSQLBlock() encountered unknown Block Type " << (int)_block << "!" << std::endl;
            }
        }
        virtual ~MetaSQLBlock() {
            while (!_items.empty())
            {
                MetaSQLOutput *tref = _items.back();
                _items.pop_back();
                if(tref)
                  delete tref;
            }
            if(_alt) {
                delete _alt;
                _alt = 0;
            }
            if (_if_func) {
                delete _if_func;
                _if_func = 0;
            }
        }

        enum Block {
            BlockGeneric = -1,
            BlockUnknown = 0,
            BlockIf,
            BlockElseIf,
            BlockElse,
            BlockForEach
        };

        bool isValid() { return _valid; }
        Block type() { return _block; }

        void append(MetaSQLOutput * mso) {
            if(mso) {
                _items.push_back(mso);
            }
        }

        void setAlternate(MetaSQLBlock * alt) {
            _alt = alt;
        }

        virtual std::string toString(MetaSQLInfo * mif, int * nBreaks = 0, bool * isContinue = 0) {
            std::string results;

            MetaSQLOutput * output = 0;
            bool b = false, myContinue = false;
            int myBreaks = 0;
            int oldPos = 0;
            unsigned int ui = 0, uii = 0;
            unsigned int lc = 0;
            switch(_block) {
                case BlockIf:
                case BlockElseIf:
                    b = _if_func->toVariant(mif, nBreaks, isContinue) == mif->trueValue();
                    if(_if_not) b = !b;
                    if(b) {
                        for(ui = 0; ui < _items.size(); ui++)
                        {
                            output = _items.at(ui);
                            results += output->toString(mif, nBreaks, isContinue);
                            if(nBreaks && *nBreaks) break;
                        }
                    } else if(_alt) {
                        results = _alt->toString(mif, nBreaks, isContinue);
                    }
                    break;

                case BlockForEach:
                    lc = mif->getValueListCount(_loopVar);
                    if(lc > 0) {
                        oldPos = mif->getValuePos(_loopVar);
                        for(ui = 0; ui < lc; ui++) {
                            mif->setValuePos(_loopVar, ui);

                            myBreaks = 0;
                            myContinue = false;

                            // execute the block
                            for(uii = 0; uii < _items.size(); uii++)
                            {
                                output = _items.at(uii);
                                results += output->toString(mif, &myBreaks, &myContinue);
                                if(myBreaks) break;
                            }

                            if(myBreaks > 0) {
                                myBreaks--;
                                if(myBreaks > 0 || !myContinue) {
                                    if(nBreaks) *nBreaks = myBreaks;
                                    if(isContinue) *isContinue = myContinue;
                                    break;
                                }
                            }
                        }
                        mif->setValuePos(_loopVar, oldPos);
                    }
                    break;

                case BlockElse:
                case BlockGeneric:
                    for(ui = 0; ui < _items.size(); ui++)
                    {
                        output = _items.at(ui);
                        results += output->toString(mif, nBreaks, isContinue);
                        if(nBreaks && *nBreaks) break;
                    }
                    break;

                default:
                    _parent->_logger << "Encountered unknown Block type " << (int)_block << "." << std::endl;
            };

            return results;
        }

    protected:
        Block identifyBlock(const std::string & block) {
            if(block == "generic")
                return BlockGeneric;
            else if(block == "if")
                return BlockIf;
            else if(block == "elseif")
                return BlockElseIf;
            else if(block == "else")
                return BlockElse;
            else if(block == "foreach")
                return BlockForEach;

            _parent->_logger << "Unable to identify block '" << block << "'!" << std::endl;

            return BlockUnknown;
        }

    private:
        bool _valid;
        Block _block;

        MetaSQLBlock * _alt;
        std::vector<MetaSQLOutput*> _items;

        std::string _loopVar;

        bool _if_not;
        MetaSQLFunction * _if_func;
};

MetaSQLQueryParser::~MetaSQLQueryParser() {
    if(_top) {
        delete _top;
        _top = 0;
    }
}

std::string MetaSQLQueryParser::populate(MetaSQLInfo * mif) {
    std::string sql;
    if(_top) {
        sql = trim(_top->toString(mif));
    }
    return sql;
}

bool MetaSQLQueryParser::parse_query(const std::string & query) {
    if(_top) {
        delete _top;
        _top = 0;
        _valid = false;
    }
    _top = new MetaSQLBlock(this, "generic", "");
    std::vector<MetaSQLBlock*> _blocks;
    _blocks.push_back(_top);
    MetaSQLBlock * _current = _top;

    std::size_t lastPos = 0;
    std::size_t currPos = 0;
    while(currPos != std::string::npos) {
        currPos = query.find_first_of("'\"-/<", currPos);
        if(currPos != std::string::npos && (query.at(currPos) == '\'' || query.at(currPos) == '"'))
        {
            std::string needle = std::string("\\") + query.at(currPos);
            currPos--; // back up one space so our next +2 iteration doesn't move us to far on the first round
            do {
                currPos += 2; // first round net move + 1, otherwise we hit an escape slash so we want to move ahead post it and the following char
                currPos = query.find_first_of(needle, currPos);
            } while(currPos != std::string::npos && query.at(currPos) == '\\');
            if(currPos != std::string::npos)
                currPos++; // we found the end of the string so move forward one more so we don't thing we are starting again
            continue; // this was just a quoted string that wanted to jump over so we don't parse out stuff inside the string
        }
        int foundWhat = 0;
        if(currPos != std::string::npos) {
            if(query.at(currPos) == '-' && query.at(currPos+1) == '-') {
                foundWhat = 1;
            } else if(query.at(currPos) == '/' && query.at(currPos+1) == '*') {
                foundWhat = 2;
            } else if(query.at(currPos) == '<' && query.at(currPos+1) == '?') {
                foundWhat = 3;
            } else {
                currPos++;
                continue; // No match so just move forward and try again
            }
        }
        if(lastPos != currPos) {
            _current->append(new MetaSQLString(this, query.substr(lastPos, (currPos==std::string::npos?currPos:currPos-lastPos))));
        }
        if(foundWhat == 1) {
            lastPos = currPos;
            currPos = query.find_first_of("\r\n", lastPos);
            _current->append(new MetaSQLComment(this, query.substr(lastPos, (currPos==std::string::npos?currPos:currPos-lastPos))));
        } else if(foundWhat == 2) {
            lastPos = currPos;
            std::string cmntStart("/*");
            std::string cmntEnd("*/");
            currPos = query.find(cmntEnd, lastPos);
            std::size_t s2 = lastPos;
            do {
                s2 = query.find(cmntStart, s2+1);
                if(s2 > lastPos && s2 < currPos) {
                    currPos = query.find(cmntEnd, currPos+1);
                }
            } while(s2 > lastPos && s2 < currPos);
            if(currPos != std::string::npos)
              currPos++;
            _current->append(new MetaSQLComment(this, query.substr(lastPos, (currPos==std::string::npos?currPos:currPos-lastPos))));
            if(currPos != std::string::npos)
              currPos++;
        } else if(foundWhat == 3) {
            lastPos = currPos + 2;
            currPos = query.find("?>", lastPos);
            std::string s = trim(query.substr(lastPos, (currPos==std::string::npos?currPos:currPos-lastPos)));
            std::string cmd, options;
            std::size_t i = s.find_first_not_of(__wordchars);
            if(i == std::string::npos) {
                cmd = s;
                options.clear();
            } else {
                cmd = s.substr(0, i);
                options = s.substr(i);
            }
            cmd = strlower(cmd);

            if(cmd == "endif" || cmd == "endforeach") {
                MetaSQLBlock::Block _block = _current->type();
                if(  (cmd == "endif" && (  _block == MetaSQLBlock::BlockIf
                                        || _block == MetaSQLBlock::BlockElseIf
                                        || _block == MetaSQLBlock::BlockElse) )
                  || (cmd == "endforeach" && ( _block == MetaSQLBlock::BlockForEach ) ) ) {
                    _blocks.pop_back();
                    _current = _blocks.back();
                } else {
                    // uh oh! We encountered an end block tag when we were either not in a
                    // block or were in a block of a different type.
                    _logger << "Encountered an unexpected " << cmd << "." << std::endl;
                    _valid = false;
                    return false;
                }
            } else if(cmd == "if" || cmd == "foreach") {
                // we have a control statement here and need to create a new block
                MetaSQLBlock * b = new MetaSQLBlock(this, cmd, options);
                if(b->isValid()) {
                    _current->append(b);
                    _blocks.push_back(b);
                    _current = b;
                } else {
                    _logger << "Failed to create new " << cmd << " block." << std::endl;
                    delete b;
                    _valid = false;
                    return false;
                }
            } else if(cmd == "elseif" || cmd == "else") {
                // we need to switch up are if block to include this new alternate
                if(_current->type() == MetaSQLBlock::BlockElse) {
                    _logger << "Encountered unexpected " << cmd << " statement within else block." << std::endl;
                    _valid = false;
                    return false;
                } else if(_current->type() != MetaSQLBlock::BlockIf && _current->type() != MetaSQLBlock::BlockElseIf) {
                    _logger << "Encountered unexpected " << cmd << " statement outside of if/elseif block." << std::endl;
                    _valid = false;
                    return false;
                } else {
                    MetaSQLBlock * b = new MetaSQLBlock(this, cmd, options);
                    if(b->isValid()) {
                        _current->setAlternate(b);
                        _blocks.pop_back();
                        _blocks.push_back(b);
                        _current = b;
                    } else {
                        _logger << "Failed to create new " << cmd << " block." << std::endl;
                        delete b;
                        _valid = false;
                        return false;
                    }
                }
            } else {
                // we must have a function... if not then i don't know what it could be.
                // first we must parse the options into a list of parameters for the function
                options = trim(options);
                std::vector<std::string> plist;
                if(!options.empty()) {
                    // first if we have a '(' then we will only parse out the information between it
                    // and the following ')'
                    char qc = options[0];
                    bool enclosed = false;
                    bool in_string = false;
                    char string_starter = '"';
                    std::string wip;
                    if(qc == '(') enclosed = true;
                    bool working = !enclosed;
                    for(std::size_t p = 0; p < options.size(); p++) {
                        qc = options.at(p);
                        if(!working && enclosed && qc == '(')
                            working = true;
                        else {
                            if(in_string) {
                                if(qc == '\\') {
                                    wip += options.at(++p);
                                } else if(qc == string_starter) {
                                    in_string = false;
                                } else {
                                    wip += qc;
                                }
                            } else {
                                if(qc == ',') {
                                    plist.push_back(wip);
                                    wip.clear();
                                } else if(isspace(qc)) {
                                    // eat white space
                                } else if(qc == '\'' || qc == '"') {
                                    in_string = true;
                                    string_starter = qc;
                                } else if(enclosed && qc == ')') {
                                    working = false;
                                    break;
                                } else {
                                    wip += qc;
                                }
                            }
                        }
                    }
                    if(!wip.empty())
                        plist.push_back(wip);
                }

                MetaSQLFunction * f = new MetaSQLFunction(this, cmd, plist);
                if(f->isValid()) {
                    _current->append(f);
                } else {
                    _logger << "Failed to create new " << cmd << " function." << std::endl;
                    delete f;
                    _valid = false;
                    return false;
                }
            }
            currPos += 2;
        }
        lastPos = currPos;
    }

    _valid = true;
    return true;
}

