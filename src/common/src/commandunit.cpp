#include "commandunit.h"

#include <QTimer>
#include <QDebug>

CommandUnit::CommandUnit(QObject *parent,
                         std::initializer_list<CommandEntity*> commands,
                         CommandEntityInfo commandInfo) :
    CommandEntity(parent, commandInfo),
    m_queue(commands),
    m_numProgressingEntities(numberOfProgressingEntities()),
    m_completedProgressingEntities(0)
{
}

CommandUnit::CommandUnit(QObject *parent,
                         std::deque<CommandEntity*> commands,
                         CommandEntityInfo commandInfo) :
    CommandEntity(parent, commandInfo),
    m_queue(commands),
    m_numProgressingEntities(numberOfProgressingEntities()),
    m_completedProgressingEntities(0)
{
}

CommandUnit::CommandUnit(QObject* parent,
                         const QQueue<CommandEntity*>& commands,
                         CommandEntityInfo commandInfo) :
    CommandEntity(parent, commandInfo)
{
    for (CommandEntity* command : commands) {
        this->m_queue.push_back(command);
    }

    this->m_numProgressingEntities = numberOfProgressingEntities();
    this->m_completedProgressingEntities = 0;
}

CommandUnit::~CommandUnit()
{
    abortAllCommands();
}

bool CommandUnit::staticProgress() const
{
    return this->m_numProgressingEntities < 1;
}

unsigned int CommandUnit::numberOfProgressingEntities()
{
    unsigned int c = 0;
    for (const CommandEntity* command : this->m_queue) {
        if (!command)
            continue;
        if (!command->staticProgress()) c++;
    }
    return c;
}

bool CommandUnit::startWork()
{
    if (!CommandEntity::startWork())
        return false;

    if (this->m_queue.empty()) {
        qWarning() << "CommandUnit is empty";
        return false;
    }

    runFirst();

    setState(RUNNING);
    Q_EMIT started();
    return true;
}

bool CommandUnit::abortWork()
{
    if (this->m_queue.empty())
        return true;

    abortAllCommands();

    Q_EMIT aborted();
    return true;
}

void CommandUnit::runNext()
{
    if (this->m_queue.empty())
        return;

    CommandEntity* command = this->m_queue.front();
    this->m_queue.pop_front();

    if (command) {
        if (!command->staticProgress())
            this->m_completedProgressingEntities++;
        qDebug() << "deleting command:" << command << command->state();
        delete command;
        qDebug() << "delete done";
    }

    runFirst();
}

void CommandUnit::runFirst()
{
    // Skip nullptrs
    while (!this->m_queue.empty() && !this->m_queue.front()) {
        qDebug() << "popping nullptr from queue";
        this->m_queue.pop_front();
    }

    // All commands successfully run
    if (this->m_queue.empty()) {
        Q_EMIT done();
        return;
    }

    QObject::connect(this->m_queue.front(), &CommandEntity::progressChanged,
                     this, &CommandUnit::updateProgress);
    QObject::connect(this->m_queue.front(), &CommandEntity::aborted,
                     this, &CommandUnit::abortWork);
    QObject::connect(this->m_queue.front(), &CommandEntity::done,
                     this, [=](){
        QTimer::singleShot(0, this, &CommandUnit::runNext);
    });

    this->m_queue.front()->run();
}

void CommandUnit::updateProgress()
{
    if (!this->m_queue.front())
        return;

    if (this->m_numProgressingEntities < 1)
        return;

    qreal partialProgress =
            ((this->m_queue.front()->progress()) / (qreal)this->m_numProgressingEntities);
    qreal progressCompleted =
            ((qreal)this->m_completedProgressingEntities / (qreal)this->m_numProgressingEntities);

    qDebug() << "partial progress:" << partialProgress;

    setProgress((qreal)(partialProgress + progressCompleted));
}

void CommandUnit::abortAllCommands()
{
    while (!this->m_queue.empty()) {
        CommandEntity* command = this->m_queue.front();
        this->m_queue.pop_front();
        if (command) {
            qDebug() << "deleting command:" << command;
            delete command;
        }
    }
}
