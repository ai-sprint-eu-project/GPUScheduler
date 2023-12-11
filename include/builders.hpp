// Copyright 2020-2021 Federica Filippini
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef BUILDERS_HH
#define BUILDERS_HH

#include <memory>

/* BaseBuilder
*     abstract builder base class to be used with object factories. It relies
*     on a constructor that takes five strings as arguments; it is intended 
*     to be used with Heuristic class
*/
template <typename BaseClass>
class BaseBuilder {

public:
  virtual std::unique_ptr<BaseClass> create (const std::string&, 
                                             const std::string&, 
                                             const std::string&, 
                                             const std::string&) = 0;
  virtual ~BaseBuilder (void) = default;

};


/* Builder
*     builder class to be used with object factories. It relies on
*     on a constructor that takes five strings as arguments; it is intended 
*     to be used with classes deriving from Heuristic
*/
template <typename DerivedClass, typename BaseClass>
class Builder: public BaseBuilder<BaseClass> {

public:
  std::unique_ptr<BaseClass> create (const std::string& s1, 
                                     const std::string& s2, 
                                     const std::string& s3, 
                                     const std::string& s4)
  {
    return std::unique_ptr<BaseClass> (new DerivedClass(s1,s2,s3,s4));
  }

};

#endif /* BUILDERS_HH */