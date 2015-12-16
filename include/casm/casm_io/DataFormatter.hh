#ifndef DATAFORMATTER_HH
#define DATAFORMATTER_HH

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <functional>
#include <boost/tokenizer.hpp>
#include "casm/CASM_global_definitions.hh"
#include "casm/container/multivector.hh"
#include "casm/misc/CASM_math.hh"
#include "casm/misc/unique_cloneable_map.hh"
#include "casm/casm_io/jsonParser.hh"
#include "casm/casm_io/DataStream.hh"
#include "casm/casm_io/FormatFlag.hh"


namespace CASM {
  
  /// \defgroup DataFormatter
  ///
  /// \brief Functions and classes related to formatting data
  ///
  
  
  class DataStream;

  template<typename DataObject>
  class BaseDatumFormatter;

  // Given expression string
  //   "subexpr1 subexpr2(subsub1) subexpr3(subsub2(subsubsub1))"
  // splits expresion so that
  //    tag_names = {"subexpr1", "subexpr2", "subexpr3"}
  //    sub_exprs = {"","subsub1", "subsub2(subsubsub1)"}
  // whitespace and commas are ignored
  void split_formatter_expression(const std::string &input_expr,
                                  std::vector<std::string> &tag_names,
                                  std::vector<std::string> &sub_exprs);

  /* A DataFormatter performs extraction of disparate types of data from a 'DataObject' class that contains
   * various types of unasociated 'chunks' of data.
   * The DataFormatter is composed of one or more 'DatumFormatters', with each DatumFormatter knowing how to
   * access and format a particular type of data from the 'DataObject'.
   * BaseDatumFormatter<DataObject> is a virtual class from which all DatumFormatters that access the particular
   * DataObject derive.
   *
   * As an example consider a Configuration object, which has numerous attributes (name, energy, composition, etc).
   * These attributes can be accessed and formatted using a DataFormatter<Configuration>, which contains a number of
   *
   * DatumFormatter<Configuration> objects.  For example
   *      NameConfigDatumFormatter        (derived from DatumFormatter<Configuration>)
   *      EnergyConfigDatumFormatter      (derived from DatumFormatter<Configuration>)
   *      CompositionConfigDatumFormatter (derived from DatumFormatter<Configuration>)
   *
   *  Example usage case:
   *        DataFormatter<DataObject> my_data_formatter;
   *        initialize_data_formatter_from_user_input(my_data_formatter, user_input);
   *        std::cout << my_data_formatter(primclex.config_begin(), primclex.config_end());
   *        my_json_parser = my_data_formatter(primclex.config_begin(), primclex.config_end());
   *
   * \ingroup DataFormatter
   */
  
  /// \brief Functions and classes related to formatting data
  ///
  /// A DataFormatter performs extraction of disparate types of data from objects 
  /// of 'DataObject' class. The DataFormatter is composed of one or more  
  /// 'DatumFormatters', with each DatumFormatter knowing how to access or 
  /// calculate and then format a particular type of data from the 'DataObject'.
  /// 
  /// BaseDatumFormatter<DataObject> is a virtual class from which all DatumFormatters 
  /// that access the particular DataObject derive.
  ///
  /// DataFormatterParser<DataObject> is a singleton that can be specialized for 
  /// creating and using a DataFormatterDictionary<DataObject> containing all of
  /// the DatumFormatter that exist for a particular DataObject.
  /// 
  ///
  /// As an example, consider a Configuration object, which has numerous attributes 
  /// (name, energy, composition, etc) which can either be accessed or calculated.
  /// These attributes can be accessed or calculated and then formatted using a 
  /// DataFormatter<Configuration>, which contains a number of DatumFormatter<Configuration> 
  /// objects. 
  ///
  /// Example DatumFormatter<Configuration> include:
  /// - ConfigData::Name             (derived from DatumFormatter<Configuration>)
  /// - ConfigData::Comp             (derived from DatumFormatter<Configuration>)
  /// - ConfigData::FormationEnergy  (derived from DatumFormatter<Configuration>)
  ///
  ///
  ///
  /// A DataFormatter<Configuration> can either be constructed explicitly with the
  /// desired set of DatumFormatter, or via a DataFormatterDictionary<Configuration> 
  /// which parses a string containing descriptions of the DatumFormatter to 
  /// include and optionally arguments for initializing the DatumParser. Additional 
  /// DatumFormatter can also be added via 'DataFormatter::push_back' or the '<<' 
  /// operator.
  ///
  /// Explicit construction example:
  /// \code
  /// DataFormatter<Configuration> formatter( ConfigData::Name(), 
  ///                                         ConfigData::Comp(),
  ///                                         ConfigData::FormationEnergy());
  /// \endcode
  ///
  /// Construction via DataFormatterDictionary and a string with arguments:
  /// \code
  /// std::string args = "name formation_energy comp(a) comp(c)"
  /// DataFormatter<Configuration> formatter = DataFormatterDictionary<Configuration>::parse(args);
  /// \endcode
  /// Calling DataFormatterDictionary::parse will for each name in the 'args' 
  /// push back the DatumFormatters with the same name and then call 
  /// BaseDatumFormatter::parse_args with the content inside the parentheses (empty 
  /// string otherwise).
  ///
  /// Adding additional DatumFormatter:
  /// \code
  /// formatter.push_back(ConfigData::Comp());
  /// formatter << ConfigData::Comp();
  /// \endcode
  ///
  /// Once a DataFormatter has been constructed, it can be used to output formatted
  /// data from a single DataObject or a range of DataObject. The output can be
  /// sent to an output stream, a DataStream, or a jsonParser. For example:
  /// \code
  /// jsonParser json;
  ///
  /// // output formatted data from a single DataObject
  /// Configuration config;
  /// std::cout << formatter(config);
  /// json = formatter(config);
  ///
  /// // output formatted data from a range of DataObject
  /// std::vector<Configuration> container = ...;
  /// std::cout << formatter(container.begin(), container.end());
  /// json = formatter(container.begin(), container.end());
  /// \endcode
  ///
  /// \ingroup DataFormatter
  ///
  template<typename DataObject>
  class DataFormatter {
    // These are private classes that sweeten the I/O syntax
    // they have to be forward declared ahead of DataFormatter::operator()
    template<typename IteratorType>
    class FormattedIteratorPair;

    class FormattedObject;
  public:
    DataFormatter(int _sep = 4, int _precision = 12, std::string _comment = "#") :
      m_initialized(false), m_prec(_precision), m_sep(_sep), m_indent(0), m_comment(_comment) {}

    template<typename ...Args>
    DataFormatter(const Args &... formatters)
      : DataFormatter() {
      push_back(formatters...);
    }

    bool empty() const {
      return m_data_formatters.size() == 0;
    }

    void clear() {
      m_data_formatters.clear();
    }

    template<typename IteratorType>
    FormattedIteratorPair<IteratorType> operator()(IteratorType begin, IteratorType end) const {
      return FormattedIteratorPair<IteratorType>(this, begin, end);
    }

    void set_indent(int _indent) {
      if(m_col_width.size() == 0)
        return;
      m_col_width[0] += _indent;
      m_col_sep[0] += _indent;
      //m_indent=_indent;
    }

    FormattedObject operator()(const DataObject &data_obj) const {
      return FormattedObject(this, data_obj);
    }

    /// Returns true if _obj has valid data for all portions of query
    bool validate(const DataObject &_obj) const;

    ///Output selected data from DataObject to DataStream
    void inject(const DataObject &_obj, DataStream &_stream) const;

    ///Output data as specified by *this of the given DataObject
    void print(const DataObject &_obj, std::ostream &_stream) const;

    ///Output data as specified by *this of the given DataObject to json with format {"name1":x, "name2":x, ...}
    jsonParser &to_json(const DataObject &_obj, jsonParser &json) const;
    
    ///Output data as specified by *this of the given DataObject to json with format {"name1":[..., x], "name2":[..., x], ...}
    jsonParser &to_json_arrays(const DataObject &_obj, jsonParser &json) const;

    ///print the header, using _tmplt_obj to inspect array sizes, etc.
    void print_header(const DataObject &_tmplt_obj, std::ostream &_stream) const;

    /// Add a particular BaseDatumFormatter to *this
    /// If the previous Formatter matches the new formatter, try to just parse the new args into it
    void push_back(const BaseDatumFormatter<DataObject> &new_formatter, const std::string &args) {
      
      //If the last formatter matches new_formatter, try to parse the new arguments into it
      if(m_data_formatters.size() > 0 && m_data_formatters.back()->name() == new_formatter.name()) {
        if(m_data_formatters.back()->parse_args(args))
          return;
      }

      m_data_formatters.push_back(new_formatter.clone());
      m_col_sep.push_back(0);
      m_col_width.push_back(0);
      m_data_formatters.back()->parse_args(args);
      
    }

    void push_back(const BaseDatumFormatter<DataObject> &new_formatter) {

      m_data_formatters.emplace_back(new_formatter);
      m_col_sep.push_back(0);
      m_col_width.push_back(0);
    }

    template<typename ...Args>
    void push_back(const BaseDatumFormatter<DataObject> &new_formatter, const Args &... formatters) {
      push_back(new_formatter);
      push_back(formatters...);
    }

    void append(const DataFormatter<DataObject> &_tail) {
      for(const auto& frmtr : _tail.m_data_formatters)
        push_back(*frmtr);
    }


    DataFormatter<DataObject> &operator<<(const BaseDatumFormatter<DataObject> &new_formatter) {
      push_back(new_formatter);
      return *this;
    }

    void set_header_prefix(const std::string &_prefix) {
      m_comment += _prefix;
    }
  private:
    mutable bool m_initialized;
    //List of all the ConfigFormatter objects you want outputted
    std::vector<notstd::cloneable_ptr<BaseDatumFormatter<DataObject> > > m_data_formatters;
    mutable std::vector<Index> m_col_sep;
    mutable std::vector<Index> m_col_width;
    //Decimal precision
    int m_prec;
    //number of spaces between columns
    int m_sep;

    int m_indent;
    //comment prefix -- default to "#"
    std::string m_comment;

    void _initialize(const DataObject &_tmplt) const;
  };

  /// \brief Abstract base class from which all other DatumFormatter<DataObject> classes inherit
  ///
  /// The job of a DatumFormatter is to access and format a particular type of 
  /// data that is stored in a <DataObject> class, which is a template paramter.
  ///
  /// \ingroup DataFormatter
  ///
  template<typename DataObject>
  class BaseDatumFormatter {
  public:

    enum FormatterType {Property, Operator};
    typedef long difference_type;
    

    BaseDatumFormatter(const std::string &_init_name, const std::string &_desc) :
      m_name(_init_name), m_description(_desc) {}

    /// Allow polymorphic deletion
    virtual ~BaseDatumFormatter() {};

    /// \brief Returns a name for the formatter, which becomes the tag used for parsing
    const std::string &name() const {
      return m_name;
    }

    /// \brief Returns a short description of the formatter and its allowed arguments (if any).
    /// This description is used to automatically generate help screens
    const std::string &description() const {
      return m_description;
    }

    virtual FormatterType type() const {
      return Property;
    }

    /// \brief Make an exact copy of the formatter (including any initialized members)
    ///
    std::unique_ptr<BaseDatumFormatter<DataObject> > clone() const {
      return std::unique_ptr<BaseDatumFormatter<DataObject> >(this->_clone());
    }

    virtual void init(const DataObject &_template_obj) const {

    };

    ///\brief Returns true if _data_obj has valid values for requested data
    ///
    /// Default implementation always returns true
    virtual bool validate(const DataObject &_data_obj) const {
      return true;
    }

    ///\brief Returns a long expression for each scalar produced by the formatter
    /// parsing the long_header should reproduce the exact query described by the formatter
    /// Ex: "clex(formation_energy)" or "comp(a)    comp(c)"
    virtual std::string long_header(const DataObject &_template_obj) const {
      return short_header(_template_obj);
    };

    ///\brief Returns a short expression for the formatter
    /// parsing the short_header should allow the formatter to be recreated
    /// (but the short header does not specify a subset of the elements)
    /// Ex: "clex(formation_energy)" or "comp"
    virtual std::string short_header(const DataObject &_template_obj) const {
      return name();
    };

    /// If data must be printed on multiple rows, returns number of rows needed to output all data from _data_obj
    /// DataFormatter class will subsequently pass over _data_obj multiple times to complete printing (if necessary)
    virtual Index num_passes(const DataObject &_data_obj) const {
      return 1;
    }

    /// Print formatted data from _data_obj to _stream, while specifying which output pass is requested
    /// If implementation does not depend on pass_index, it may safely be ignored
    virtual void print(const DataObject &_data_obj, std::ostream &_stream, Index pass_index = 0) const = 0;

    /// Stream selected data from _data_obj to _stream, while specifying which output pass is requested
    /// If implementation does not depend on pass_index, it may safely be ignored
    virtual void inject(const DataObject &_data_obj, DataStream &_stream, Index pass_index = 0) const = 0;

    /// Assumes that 'json' object is simply assigned, and it is the job of DataFormatter (or  some other managing entity)
    /// to pass the correct 'json' object.
    ///     Ex:  DerivedDatumFormatter my_formatter;
    ///          initialize(my_formatter); // does some set of initialization steps
    ///          jsonParser my_big_data_object;
    ///          my_formatter.to_json(my_data_object, my_big_data_object["place_to_write"]["my_formatter_data"]);
    virtual jsonParser &to_json(const DataObject &_data_obj, jsonParser &json)const = 0;
    
    /// If DatumFormatter accepts arguments, parse them here.  Arguments are assumed to be passed from the command line
    /// via:         formattername(argument1,argument2,...)
    ///
    /// from which DerivedDatumFormatter::parse_args() receives the string "argument1,argument2,..."
    /// Returns true if parse is successful, false if not (e.g., takes no arguments, already initialized, malformed input, etc).
    virtual bool parse_args(const std::string &args) {
      return args.size() == 0;
    }
  protected:
    typedef multivector<Index>::X<2> IndexContainer;

    /// Derived DatumFormatters have some optional functionality for parsing index 
    /// expressions in order to make it easy to handle ranges such as:
    /// \code
    ///       formatter_name(3,4:8)
    /// \endcode
    /// in which case, DerivedDatumFormatter::parse_args() is called with the string "3,4:8"
    /// by dispatching that string to BaseDatumFormatter::_parse_index_expression(), 
    /// m_index_rules will be populated with {{3,4},{3,5},{3,6},{3,7},{3,8}}
    void _parse_index_expression(const std::string &_expr);
    
    void _add_rule(const std::vector<Index> &new_rule) const {
      m_index_rules.push_back(new_rule);
    }
    
    const IndexContainer &_index_rules() const {
      return m_index_rules;
    }
  
  private:
    
    /// \brief Make an exact copy of the formatter (including any initialized members)
    ///
    virtual BaseDatumFormatter* _clone() const = 0;
    /**{ return notstd::make_unique<DerivedDatumFormatter>(*this);}**/
    
    std::string m_name;
    std::string m_description;
    mutable IndexContainer  m_index_rules;
    
  };

  template<typename T>
  bool always_true(const T &) {
    return true;
  };


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /// \brief Abstract base class to enable generic formatting
  ///
  /// \ingroup DataFormatter
  ///
  class FormattedPrintable {
    
    // Couldn't figure out how to get compiler to correctly substitute names
    // virtual "FormattedPrintable" class is a workaround
  
  public:
    virtual ~FormattedPrintable() {}
    virtual void inject(DataStream &stream)const = 0;
    virtual void print(std::ostream &stream)const = 0;
    virtual jsonParser &to_json(jsonParser &json)const = 0;
  };


  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// \brief Implements generic formatting member functions for ranges of data objects
  ///
  /// \ingroup DataFormatter
  ///
  template<typename DataObject> template<typename IteratorType>
  class DataFormatter<DataObject>::FormattedIteratorPair : public FormattedPrintable {
    DataFormatter<DataObject> const *m_formatter_ptr;
    IteratorType m_begin_it, m_end_it;
  public:
    FormattedIteratorPair(const DataFormatter<DataObject> *_formatter_ptr, IteratorType _begin, IteratorType _end):
      m_formatter_ptr(_formatter_ptr), m_begin_it(_begin), m_end_it(_end) {}

    void inject(DataStream &_stream) const {
      if(m_begin_it == m_end_it)
        return;
      // hack: always print header to initialize things, like Clexulator, but in this case throw it away
      std::stringstream _ss;
      m_formatter_ptr->print_header(*m_begin_it, _ss);
      for(IteratorType it(m_begin_it); it != m_end_it; ++it)
        m_formatter_ptr->inject(*it, _stream);
    }

    void print(std::ostream &_stream) const {
      if(m_begin_it == m_end_it)
        return;

      FormatFlag format(_stream);
      if(format.print_header()) {
        m_formatter_ptr->print_header(*m_begin_it, _stream);
      }
      else { // hack: always print header to initialize things, like Clexulator, but in this case throw it away
        std::stringstream _ss;
        m_formatter_ptr->print_header(*m_begin_it, _ss);
      }
      format.print_header(false);
      _stream << format;
      for(IteratorType it(m_begin_it); it != m_end_it; ++it)
        m_formatter_ptr->print(*it, _stream);
    }

    jsonParser &to_json(jsonParser &json) const {
      json.put_array();
      for(IteratorType it(m_begin_it); it != m_end_it; ++it)
        json.push_back((*m_formatter_ptr)(*it));
      return json;
    }
    
    ///Output data with format {"name1":[..., x], "name2":[..., x], ...}
    jsonParser &to_json_arrays(jsonParser &json) const {
      json = jsonParser::object();
      for(IteratorType it(m_begin_it); it != m_end_it; ++it)
        m_formatter_ptr->to_json_arrays(*it, json);
      return json;
    }


  };

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// \brief Implements generic formatting member functions for individual data objects
  ///
  /// \ingroup DataFormatter
  ///
  template<typename DataObject>
  class DataFormatter<DataObject>::FormattedObject : public FormattedPrintable {
    DataFormatter<DataObject> const *m_formatter_ptr;
    DataObject const *m_obj_ptr;
  public:
    FormattedObject(const DataFormatter<DataObject> *_formatter_ptr, const DataObject &_obj):
      m_formatter_ptr(_formatter_ptr), m_obj_ptr(&_obj) {}

    void inject(DataStream &_stream) const {
      m_formatter_ptr->inject(*m_obj_ptr, _stream);
    }

    void print(std::ostream &_stream) const {
      FormatFlag format(_stream);
      if(format.print_header())
        m_formatter_ptr->print_header(*m_obj_ptr, _stream);
      format.print_header(false);
      _stream << format;

      m_formatter_ptr->print(*m_obj_ptr, _stream);
    }

    jsonParser &to_json(jsonParser &json) const {
      m_formatter_ptr->to_json(*m_obj_ptr, json);
      return json;
    }
    
  };

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
  template<typename DataObject, typename DatumFormatterType>
  struct DictionaryConverter {
    
    typedef DatumFormatterType formatter;
    
    
    // performs a static_cast of value.clone().unique().release() 
    // (i.e. a BaseDatumFormatter*) to formatter*
    
    notstd::cloneable_ptr<formatter> operator()(const formatter& value){
      return notstd::cloneable_ptr<formatter>(static_cast<formatter*>(value.clone().release()));
    }
  
  };
  
  template<typename DataObject>
  struct DictionaryConverter<DataObject, BaseDatumFormatter<DataObject> > {
    
    typedef BaseDatumFormatter<DataObject> formatter;
    
    notstd::cloneable_ptr<formatter> operator()(const formatter& value){
      return notstd::cloneable_ptr<formatter>(value);
    }
  
  };
  
    
  /// \brief  Parsing dictionary for constructing a DataFormatter<DataObject> object.
  ///
  /// \ingroup DataFormatter
  ///
  template<typename DataObject, typename DatumFormatterType = BaseDatumFormatter<DataObject> >
  class DataFormatterDictionary : 
    public notstd::unique_cloneable_map<std::string, DatumFormatterType> {
  
  public:
  
    typedef notstd::unique_cloneable_map<std::string, DatumFormatterType> UniqueMapType;
    typedef typename UniqueMapType::key_type key_type;
    typedef typename UniqueMapType::value_type value_type;
    typedef typename UniqueMapType::size_type size_type;
    typedef typename UniqueMapType::iterator iterator;
    typedef typename UniqueMapType::const_iterator const_iterator;
    
    DataFormatterDictionary() :
      UniqueMapType([=](const value_type& value){
                      return value.name();
                    },
                    DictionaryConverter<DataObject, DatumFormatterType>() ) {}
    
    /*
    /// \brief Construct from one Formatter
    explicit DataFormatterDictionary(const value_type& formatter) :
      DataFormatterDictionary() {
      insert(formatter);
    }
    
    /// \brief Construct from many Formatter
    template<typename... Formatters>
    explicit DataFormatterDictionary(const Formatters&... more) :
      DataFormatterDictionary() {
      insert(more...);
    }
    
    /// \brief Copy constructor
    DataFormatterDictionary(const DataFormatterDictionary& dict) :
      UniqueMapType(dict) {
    }
    
    /// \brief Construct many Dictionary
    template<typename... Formatters>
    DataFormatterDictionary(const Formatters&... more) :
      DataFormatterDictionary() {
      insert(more...);
    }
    */
    
    using UniqueMapType::insert;
    
    /// \brief Equivalent to find, but throw error with suggestion if _name not found 
    const_iterator lookup(const key_type &_name) const;

    void print_help(std::ostream &_stream,
                    typename BaseDatumFormatter<DataObject>::FormatterType ftype,
                    int width, 
                    int separation) const;
    
    /// \brief Use the vector of strings to build a DataFormatter<DataObject>
    DataFormatter<DataObject> parse(const std::string &input)const;
    
    /// \brief Use a single string to build a DataFormatter<DataObject>
    DataFormatter<DataObject> parse(const std::vector<std::string> &input)const;
    
  };
  

  // ******************************************************************************
  
  /// \brief Dictionary of all DatumFormatterOperator
  template<typename DataObject>
  DataFormatterDictionary<DataObject> make_operator_dictionary();
  
  /// \brief Dictionary of all AttributeFormatter (i.e. BaseValueFormatter<V, DataObject>)
  template<typename DataObject>
  DataFormatterDictionary<DataObject> make_attribute_dictionary();
  
  /// \brief Template to can be specialized for constructing dictionaries for particular DataObject 
  ///
  /// Default includes the make_attribute_dictionary() and make_operator_dictionary()
  template<typename DataObject>
  DataFormatterDictionary<DataObject> make_dictionary() {
    DataFormatterDictionary<DataObject> dict;
    
    dict.insert(
      make_attribute_dictionary<DataObject>(),
      make_operator_dictionary<DataObject>()
    );
    
    return dict;
  }

  /// \brief A singleton for creating and using a DataFormatterDictionary<DataObject>
  template<typename DataObject>
  class DataFormatterParser {
    
  public:
    
    static const BaseDatumFormatter<DataObject> &lookup(const std::string &_name) {
      return *dictionary().lookup(_name);
    }

    static DataFormatter<DataObject> parse(const std::string &input) {
      return dictionary().parse(input);
    }

    static DataFormatter<DataObject> parse(const std::vector<std::string> &input) {
      return dictionary().parse(input);
    }

    static void add_custom_formatter(const BaseDatumFormatter<DataObject> &new_formatter) {
      dictionary().insert(new_formatter);
    }

    static void load_aliases(const fs::path &alias_path);

    static void print_help(std::ostream &_stream,
                           typename BaseDatumFormatter<DataObject>::FormatterType ftype = BaseDatumFormatter<DataObject>::Property,
                           int width = 60, int separation = 8) {
      dictionary().print_help(_stream, ftype, width, separation);
    }
    
    static DataFormatterDictionary<DataObject>& dictionary()
    {
        // Guaranteed to be destroyed, instantiated on first use.
        static DataFormatterDictionary<DataObject> m_dict = make_dictionary<DataObject>(); 
                              
        return m_dict;
    }
    
  private:
    
    /// \brief Constructor
    DataFormatterParser() {}

    /// \brief Prevent creating copies
    DataFormatterParser(const DataFormatterParser&) = delete;
    
    /// \brief Prevent creating copies
    void operator=(const DataFormatterParser&)  = delete;
  };
  
  
  // ******************************************************************************

  inline jsonParser &to_json(const FormattedPrintable &_obj, jsonParser &json) {
    return _obj.to_json(json);
  }
  
  // ******************************************************************************
  inline std::ostream &operator<<(std::ostream &_stream, const FormattedPrintable &_formatted) {
    _formatted.print(_stream);
    return _stream;
  }

  // ******************************************************************************
  inline DataStream &operator<<(DataStream &_stream, const FormattedPrintable &_formatted) {
    _formatted.inject(_stream);
    return _stream;
  }


}

#include "casm/casm_io/DataFormatter_impl.hh"

#endif
