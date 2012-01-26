#ifndef XAPP_LOCAL_CONFIG_H_
#define XAPP_LOCAL_CONFIG_H_

#include <string>

namespace XAPP
{

/**
 * Represents the storage of local configuration for an application. Local means that it is stored in the client
 * (in the future there might be server side configuration). Get the LocalConfig object by calling GetLocalConfig() on the App object.
 * Configuration is stored in a key/value manner. A key may have multiple values.
 */
class LocalConfig
{
public:
  /**
   * Sets and stores a value in the configuraion file.
   * 
   * @param key the key identifier of the value to store
   * @param value the actual value to store
   */
  void SetValue(const std::string& key, const std::string& value);
  
  /**
   * Retrieves a value from the configuration file. If the key has multiple values, you can specify the index
   * of the of key between curly brackets. For example, if the key "friends" has multiple values, you can retreive
   * the 5th element by requesting key: friend{4}
   * 
   * @param key the key identifier of the value to retrieve
   */
  std::string GetValue(const std::string& key);
  
  /**
   * Resets and deletes a value from the configuration file.
   * 
   * @param key the key identifier of the value to reset
   */
  void Reset(const std::string& key);
  
  /**
   * Resets and deletes all values from the configuration file.
   */
  void ResetAll();
  
  /**
   * Adds value to a key. Add the value to the back of the list of values.
   * 
   * @param key the key identifier of the value to store
   * @param value the actual value to store
   * @param limit maximum number of values allowed for this key. if the number is passed, values will be chopped from the front.
   */
  void PushBackValue(const std::string& key, const std::string& value, const int limit = 0);

  /**
   * Adds value to a key. Add the value to the front of the list of values.
   * 
   * @param key the key identifier of the value to store
   * @param value the actual value to store
   * @param limit maximum number of values allowed for this key. if the number is passed, values will be chopped from the back.
   */  
  void PushFrontValue(const std::string& key, const std::string& value, const int limit  = 0);
  
  /**
   * Imploes all the values of a key to a single string, glued together.
   * 
   * @param glue the glue string between values
   * @param key the key identifier of the value to retrieve
   */
  std::string Implode(const std::string glue, const std::string& key);
};

}

#endif /* LOCAL_CONFIG_H_ */
