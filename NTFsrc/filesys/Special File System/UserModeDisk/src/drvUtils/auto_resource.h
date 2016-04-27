#ifndef UTILS_AUTO_RESOURCE_H_INCLUDED
#define UTILS_AUTO_RESOURCE_H_INCLUDED
namespace utils
{
    template<class Resource>
    class AutoResource
    {
        Resource& resource_;
        AutoResource();
        AutoResource(AutoResource&);
		AutoResource& operator=(const AutoResource&);
    public:
        AutoResource(Resource& resource)
            :resource_(resource)
        {
            resource_.enter();
        }
        ~AutoResource()
        {
            resource_.leave();
        }
    };

}//namespace utils
#endif
