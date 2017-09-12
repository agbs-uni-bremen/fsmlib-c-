#ifndef IOTRACECONTAINER_H
#define IOTRACECONTAINER_H

#include "fsm/IOTrace.h"

class IOTraceContainer
{
private:
    std::shared_ptr<std::vector<IOTrace>> list;
    const std::shared_ptr<FsmPresentationLayer> presentationLayer;
public:
    IOTraceContainer(const std::shared_ptr<FsmPresentationLayer> presentationLayer);
    IOTraceContainer(std::shared_ptr<std::vector<IOTrace>>& list, const std::shared_ptr<FsmPresentationLayer> presentationLayer);
    std::shared_ptr<std::vector<IOTrace>> getList() const;
    void addUnique(IOTrace& trc);
    void add(IOTrace& trc);

    /**
     * Output the IOTraceContainer to a standard output stream
     * @param out The standard output stream to use
     * @param ot The IOTraceContainer to print
     * @return The standard output stream used, to allow user to cascade <<
     */
    friend std::ostream & operator<<(std::ostream & out, const IOTraceContainer & iot);
};

#endif // IOTRACECONTAINER_H
