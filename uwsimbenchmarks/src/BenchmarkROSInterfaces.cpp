#include "BenchmarkROSInterfaces.h"


ROSTrigger::ROSTrigger(std::string topic, TopicTrigger *trigger): ROSSubscriberInterface(topic) {
  this->trigger=trigger;
}

void ROSTrigger::createSubscriber(ros::NodeHandle &nh) {
  sub_ = nh.subscribe<std_msgs::Bool>(topic, 10, &ROSTrigger::processData, this);
}

void ROSTrigger::processData(const std_msgs::Bool::ConstPtr& msg) {
  if(msg->data==1)
    trigger->start();
  else{
    trigger->stop();
    //std::cout<<"TIMER: "<<timer->getMeasure()<<std::endl; //Testing
  }
}
ROSTrigger::~ROSTrigger(){}

ROSArrayToEuclideanNorm::ROSArrayToEuclideanNorm(std::string topic): ROSSubscriberInterface(topic) {
  newMeasure=0;
}

void ROSArrayToEuclideanNorm::createSubscriber(ros::NodeHandle &nh) {
  sub_ = nh.subscribe<std_msgs::Float32MultiArray>(topic, 10, &ROSArrayToEuclideanNorm::processData, this);
}

void ROSArrayToEuclideanNorm::processData(const std_msgs::Float32MultiArray::ConstPtr& msg) {
   int i=0;
   estimated.resize(msg->data.size());
   for(std::vector<float>::const_iterator it = msg->data.begin(); it != msg->data.end(); ++it)
     estimated[i++]=*it;
   newMeasure=1;
}

int ROSArrayToEuclideanNorm::getVector(std::vector<double> &estim){
  estim=estimated;
  if(newMeasure==1){
    newMeasure=0;
    return 1;
  }
  return 0;
}

ROSArrayToEuclideanNorm::~ROSArrayToEuclideanNorm(){}


/*ROSTopicToShapeShifter::ROSTopicToShapeShifter(std::string topic): ROSSubscriberInterface(topic) {
  std::cout<<"CREADITO "<<topic<<std::endl;
}

void ROSTopicToShapeShifter::createSubscriber(ros::NodeHandle &nh) {
  sub_ = nh.subscribe<topic_tools::ShapeShifter>(topic, 10, &ROSTopicToShapeShifter::processData, this);
}

void ROSTopicToShapeShifter::processData(const  topic_tools::ShapeShifter::ConstPtr& msg) {
  std::cout<<"************NEW DATA**********"<<std::endl<<"datatype: "<<msg->getDataType()<<" definition: "<< msg->getMessageDefinition()<<std::endl;
}

const char* ShapeShifterGetDataType(const topic_tools::ShapeShifter::ConstPtr message)
  {
      string info = message->getDataType();

      char* result = new char[info.size() + 1];
      strcpy(result, info.c_str());
      return result;  
  }
  const char* ShapeShifterGetDefinition(const topic_tools::ShapeShifter::ConstPtr message)
  {
      string info = message->getMessageDefinition();

      char* result = new char[info.size() + 1];
      strcpy(result, info.c_str());
      return result;
  }
  unsigned char* ShapeShifterGetData(const topic_tools::ShapeShifter::ConstPtr message)
  {
      unsigned char* data = new unsigned char[message->size()];

      ros::serialization::OStream stream(data, message->size());
      message->write(stream);

      return data;
  }
  unsigned int ShapeShifterGetDataLength(const topic_tools::ShapeShifter::ConstPtr message)
  {
      return message->size();
  }

ROSTopicToShapeShifter::~ROSTopicToShapeShifter(){}*/

BenchmarkInfoToROSString::BenchmarkInfoToROSString(std::string topic, int rate) :
    ROSPublisherInterface(topic, rate){
  stringToPublish="";
}

void BenchmarkInfoToROSString::createPublisher(ros::NodeHandle &nh){
  ROS_INFO("Benchmark information publisher on topic %s", topic.c_str());
  pub_ = nh.advertise < std_msgs::String > (topic, 1);
}

void BenchmarkInfoToROSString::publish(){
  std_msgs::String msg;
  msg.data=stringToPublish;
  pub_.publish(msg);
  stringToPublish="";
}

void BenchmarkInfoToROSString::changeMessage(std::string newString){
  stringToPublish=newString;
}

BenchmarkInfoToROSString::~BenchmarkInfoToROSString(){
}


CurrentToROSWrenchStamped::CurrentToROSWrenchStamped(std::string topic, int rate,   boost::shared_ptr<Current> current, SimulatedIAUV *  vehicle):
    ROSPublisherInterface(topic, rate){
  this->current=current;
  this->vehicle=vehicle;
}

void CurrentToROSWrenchStamped::createPublisher(ros::NodeHandle &nh){
  ROS_INFO("Current to Wrench Stamped publisher on topic %s", topic.c_str());
  pub_ = nh.advertise < geometry_msgs::WrenchStamped > (topic, 1);
}

void CurrentToROSWrenchStamped::publish(){
  geometry_msgs::WrenchStamped msg;
  double velocity[3];

  current->getCurrentVelocity(velocity);

  msg.header.stamp = getROSTime();
  msg.header.frame_id ="Currents";

  //Drag equation
  // Fd=0.5 * p * v^2 * Cd * A
  // p= fluid density
  // v= relative velocity
  // Cd= drag coefficient
  // A= cross-sectional area
  msg.wrench.force.x=500*velocity[0]*velocity[0]*0.82*1;
  msg.wrench.force.y=500*velocity[1]*velocity[1]*0.82*1;
  msg.wrench.force.z=500*velocity[2]*velocity[2]*0.82*1;

  msg.wrench.torque.x=0;
  msg.wrench.torque.y=0;
  msg.wrench.torque.z=0;


  pub_.publish(msg);
}

CurrentToROSWrenchStamped::~CurrentToROSWrenchStamped(){
}

BenchmarkResultToROSFloat32MultiArray::BenchmarkResultToROSFloat32MultiArray(std::string topic, int rate) :
    ROSPublisherInterface(topic, rate){
  toPublish.clear();
  publishing=0;
}

void BenchmarkResultToROSFloat32MultiArray::createPublisher(ros::NodeHandle &nh){
  ROS_INFO("Benchmark Results publisher on topic %s", topic.c_str());
  pub_ = nh.advertise < std_msgs::Float32MultiArray > (topic, 1);
}

void BenchmarkResultToROSFloat32MultiArray::publish(){
  std_msgs::Float32MultiArray msg;
  msg.data.clear();
  publishing=1;
  for(int i=0;i<toPublish.size();i++)
    msg.data.push_back(toPublish[i]);
  toPublish.clear();
  publishing=0;
  pub_.publish(msg);
}

void BenchmarkResultToROSFloat32MultiArray::newDataToPublish(float reference, float measure, float time){
  while (publishing);
  toPublish.push_back(reference);
  toPublish.push_back(measure);
  toPublish.push_back(time);
}

BenchmarkResultToROSFloat32MultiArray::~BenchmarkResultToROSFloat32MultiArray(){
}


ROSServiceTrigger::ROSServiceTrigger(std::string service,ServiceTrigger * trigger) {
  this->trigger=trigger;

  serv= nh.advertiseService( service, &ROSServiceTrigger::callback,this);
}

bool ROSServiceTrigger::callback(std_srvs::Empty::Request& request, std_srvs::Empty::Response& response) {
  trigger->start();
  return true;
}

ROSServiceTrigger::~ROSServiceTrigger(){}

