## AIOUring

### Описание

Фреймворк для работы с io_uring, асинхронными вызовами ядра Linux, [Wikipedia](https://en.wikipedia.org/wiki/Io_uring).

### Начало работы

Пример:

```c++
#include <hpuring/AIOUring.h>

int main() {
    try
    {
        AIOUring aioUring{};

        aioUring.setup();

        aioUring.pushTask(aioUring.newTask<MainTask>(&aioUring));

        return aioUring.run();
    }
    catch(std::exception &e)
    {
        logger::ERROR("Uring: " + std::string(e.what()));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

Как видно из примера - создается класс `AIOUring`, далее производится первичная настройка io_uring, посредствам `AIOUring::setup()`, при которой создаются все необходимые для работы сущности, в очередь кладется асинхронная задача `MainTask`, аналог `main()` как в обычной программе, и в конце запускается обработка очереди io_uring посредствам функции `AIOUring::run()`.

Пример `MainTask`:

```c++
class MainTask final : public AIOUringTask {
public:
    explicit MainTask(AIOUring *aioUring)
        : aioUring(aioUring) {
        config = vsbconfig::getConfig();
    }

    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        AWAIT_TASK(tcpListeningTask,
                   aioUring,
                   config.port,
                   config.maxBacklog);

        if(TASK_HAS_ERROR(tcpListeningTask)) {
            kklogging::ERROR(fmt::format("TCPListeningTask: {}", TASK_ERROR_TEXT(tcpListeningTask)));
            HPURING_SHUTDOWN(1);
        }

        kklogging::WARN("TCPListeningTask completed successfully.");

        return TASK_RESULT_NONE();
    }
private:
    AIOUring *aioUring{nullptr};
    vsbconfig::MainConfiguration config{};
    TASK_DEF(TCPListeningTask<BalancerAcceptTask>, tcpListeningTask);
};
```

Как видно из примера:

- Задача должна быть унаследована от абстрактного класса `AIOUringTask`.
- Класс должен быть `final`.
- Метод poll должен быть переопределён и должен возвращать `TaskFuture`.
- Если в методе poll используется вызов других задач или вызов операций io_uring то такой метод должен начинаться с `ASYNC_IO;`.
- Задачи, которые вызываются из метода poll должны быть сначала объявлены при помощи `TASK_DEF`, как в примере `TASK_DEF(TCPListeningTask<BalancerAcceptTask>, tcpListeningTask);`.
- **В МЕТОДЕ poll НЕ ДОЛЖНО БЫТЬ НИ ОДНОЙ ПЕРЕММЕННОЙ ОБЪЯВЛЕННОЙ НА СТЕКЕ, ТО ЕСТЬ В САМОМ МЕТОДЕ poll.** Это значит, что любые переменные которые нужно будет использовать в методе poll - нужно объявлять непосредственно в классе задачи и инициализировать либо в конструкторе, либо по ходу выполнения кода. То есть сам класс задачи и является контекстом для любой переменной, которая используется в методе poll.
- Параметром метода poll является io_result, в который записывается результат выполнения последней операции из io_uring.

### TaskFuture:

Представляет собой тип tuple `std::tuple<std::optional<AIOUringOp>, std::optional<std::any>>;`. Где первый элемент является операцией io_uring, если такая указана, а второй элемент это результат выполнения задачи, то есть тип возвращаемого задачей значения.

По умолчанию тип возвращаемого задачей значения является `std::monostate`, и когда при помощи макроса `TASK_RESULT_NONE()` задача возвращает значение, то таким значением является [std::monostate](https://en.cppreference.com/w/cpp/utility/variant/monostate), то есть пустым типом ([Unit type](https://en.wikipedia.org/wiki/Unit_type)), как void.

Чтобы переопределить возвращаемое значение, то есть, чтобы вернуть, например, строку то нужно начало задачи дополнить следующим определением:

```c++
class XTask final : public AIOUringTask {
public:
    using TResult = std::string;
    ...
```

Таким образом мы сообщаем фреймворку, что данная задача возвращает значение типа `std::string`.

### Для возвращения результата из задачи используются следующие макросы:
- `TASK_RESULT` - возвращает результат из задачи. Пример:
```c++
return TASK_RESULT(tcpSocket);
```
- `TASK_RESULT_NONE` - возвращает пустой результат. Пример:
```c++
return TASK_RESULT_NONE();
```
- `TASK_ERROR` - возвращает ошибку из задачи. Пример:
```c++
return TASK_ERROR(fmt::format("Error on tcp connection: {}", uexcept::errnoStr(-socketErrno)));
```

### Для работы с результатом выполнения задачи используются следующие макросы:
- `TASK_HAS_ERROR` - проверяет завершалась ли задача ошибкой. Пример:
```c++
if(TASK_HAS_ERROR(tcpListeningTask)) {
            kklogging::ERROR(fmt::format("TCPListeningTask: {}", TASK_ERROR_TEXT(tcpListeningTask)));
        }
```
- `TASK_ERROR_TEXT` - возвращает текст ошибки с которой завершались задача. Пример:
```c++
if(TASK_HAS_ERROR(tcpListeningTask)) {
            kklogging::ERROR(fmt::format("TCPListeningTask: {}", TASK_ERROR_TEXT(tcpListeningTask)));
        }
```
- `TASK_HAS_RESULT` - проверяет имеет ли завершенная задача результат выполнения. Пример:
```c++
if(!TASK_HAS_RESULT(tcpConnectTask))
        {
            kklogging::ERROR("No socket on connection.");
            return TASK_RESULT_NONE();
        }
```
- `TASK_RESULT_VALUE` - возвращает результат выпонения задачи. Пример:
```c++
targetSocket = TASK_RESULT_VALUE(tcpConnectTask);
```
- `TASK_HAS_OPTIONAL_RESULT` - проверяет содержит ли задача опциональный результат. Пример:
```c++
if(TASK_HAS_OPTIONAL_RESULT(resolveHostTask)) {
            resolveResult = std::move(TASK_OPTIONAL_VALUE(resolveHostTask));
        } else {
            return TASK_ERROR(fmt::format("No ip address for hostname {}", hostname));
        }
```
- `TASK_OPTIONAL_VALUE` - возвращает значение опционального результата выполнения задачи. Пример:
```c++
if(TASK_HAS_OPTIONAL_RESULT(resolveHostTask)) {
            resolveResult = std::move(TASK_OPTIONAL_VALUE(resolveHostTask));
        } else {
            return TASK_ERROR(fmt::format("No ip address for hostname {}", hostname));
        }
```
### Запуск задачи

Для запуска задачи используются следующие макросы:

- `AWAIT_TASK` - используется в случаях при которых мы будем заново возвращаться к этому месту используя циклические конструкции, или же задачу мы запускаем в единственном месте метода poll(). Первым параметром идёт имя переменной задачи которая была объявлена при помощи TASK_DEF, следующими параметрами идут параметры к конструтору задачи, либо ничего, если конструктор задачи не использует параметры. Пример:
```c++
AWAIT_TASK(findTargetTask, aioUring, requestTokens, urlTokens, queryTokens);
```

- `AWAIT_TASKNL` - используется в тех случаях когда задачу необходимо вызвать в нескольких местах вызова poll(), но такой запуск задачи исключает циклическое обращение именно к месту начала выполнения задачи. Например, в логике метода poll может быть несколько мест, где разработчик считает необходимым закрыть сокет. Пример:
```c++
AWAIT_TASKNL(sendBalancerResponse, aioUring,
                         urlTokens, tcpBuffer, clientSocket);
```

- `AWAIT_LONG_TASK` - используется в случае если есть код, который выполняется долго по тем или иным причинам, или же используется библиотека, которая не работает через io_uring, обращаясь к ресурсам. Такие задачи выполняются в отдельных потоках, без участия io_uring. В io_uring поступает лишь уведомление о завершении такой задачи. Первым параметром передается уникальное имя, относящееся к данному выову, вторым параметром передается `std::function<void(tf::Executor *executor)>`. tf::Executor - [Taskflow](https://github.com/taskflow/taskflow). Пример:
```c++
AWAIT_LONG_TASK(postgresqlUri, [this](tf::Executor *executor) {
                    setPostgresqlUriTarget();
                });
```

- `ASYNC_CONTINUE_TASK` - переносит выполнение программы на начало выполнения задачи, то есть задача выполняется повторно. Пример:
```c++
AWAIT_TASK(findTargetTask, aioUring, requestTokens, urlTokens, queryTokens);

if(TASK_HAS_ERROR(findTargetTask)) {
    logger:ERROR("task error");
    ASYNC_CONTINUE_TASK(findTargetTask);
}
logger:ERROR("task ok");
```

### Очищение ресурсов задачи

Если возникает необходимость в конце задачи - очистить какие то ресурсы: закрыть сокеты, файлы и т.п., то можно использовать следующие методы:

- free - это синхронный метод очистки ресурсов задачи, его необходимо переопределить и вместе с ним вызвать такой же метод super класса. Пример:
```c++
    void free() override {
        AIOUringTask::free();
        freeaddrinfo(host->ar_result);
        ::free(reinterpret_cast<void *>(const_cast<
                addrinfo *>(host->ar_request)));
        ::free(host);
    }
```

- finally - это асинхронный метод очистки ресурсов задачи, так же как и метод poll должен вернуть TaskFuture, и к нему применяются те же правила, которые описаны выше для poll(). Пример:
```c++
    TaskFuture finally(int io_result) override {
        ASYNC_IO;

        AWAIT_TASKNL(tcpShutAndClose, clientSocket, targetSocket);

        return TASK_RESULT_NONE();
    }
```

Оба вышеописанных метода выполняются после полного завершения задачи.

### Работа с IO операциями через io_uring

Вся работа с операциями io_uring выполняется внутри задачи посредством следующих макросов:

- AWAIT_OP - Первым параметром идёт название IO метода, которое соответствует доступному в io_uring, вторым параметром идёт название уникальной метки, третьим и последующими параметрами идут уже параметры к самому вызову, если они для такого вызова есть. Пример:
```c++
AWAIT_OP(Write, writeTo, tcpTo, buffer.data() + offset, bytesToWrite);
```

- ASYNC_CONTINUE_OP - возвращает задачу к повторному выполнения участка кода по указанной метке. Пример:
```c++
if(newLinePos == std::string_view::npos) {
            ASYNC_CONTINUE_OP(readClient);
}
```

Все операции декларируются и имплементируются в файлах AIOUringOp.h и AIOUringOp.cpp, пример для операции [Accept](https://man7.org/linux/man-pages/man2/accept.2.html):

`AIOUringOp.h`:
```c++
    static AIOUringOp Accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags = 0);
```

`AIOUringOp.cpp`:
```c++
AIOUringOp AIOUringOp::Accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags) {
    return AIOUringOp {
            .submit = [=](io_uring *ring, __u64 ptrTask) {
                struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
                io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
                sqe->user_data = ptrTask;
            }
    };
}
```

### Остановка AIOUring для завершения всего приложения

- HPURING_SHUTDOWN - данный макрос запускает операцию ShutdownUring и первым параметром передает код завершения приложения (process exit code). Пример:  
```c++
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        AWAIT_TASK(xxx);

        if(TASK_HAS_ERROR(xxx)) {
            kklogging::ERROR("critical errr");
            HPURING_SHUTDOWN(1);
        }
        ...
    }
```

### Вспомогательные макросы

- AWAIT_POLL - переносит работу асинхронной функции poll/finally к началу выполнения. Пример:
```c++
    TaskFuture poll(int io_result) override {
        ASYNC_IO;

        if(!sockets.empty()) {

            socketValue = sockets.front();

            if(socketValue >= 0) {
                AWAIT_OP(Shutdown, shutSocket, socketValue);
                AWAIT_OP(Close, closeSocket, socketValue);
            }

            sockets.pop_front();

            AWAIT_POLL();
        }

        return TASK_RESULT_NONE();
    }
```
- AWAIT_EVENT - ожидает получения события на eventfd задачи. Пример:
```c++
AWAIT_EVENT(*this->getTaskfd().lock());
```
- EVENT_NOTIFY - синхронно отправляет событие на eventfd. Пример:
```c++
EVENT_NOTIFY(eventFd);
```
- EVENT_NOTIFY_ASYNC - асинхронно отправляет событие на eventfd. Пример:
```c++
EVENT_NOTIFY_ASYNC(eventFd);
```
