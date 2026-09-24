# HW3D
Данный проект является 2й лабораторной работой на курсе по C++ для 2го курса бакалавриата от Константина Игоревича Владимирова.

В данной работе нужно реализовать эффективный поиск пересечений плоских треугольников в трёхмерном пространстве.

На вход программа получает до миллиона треугольников. Полный перебор всех пар имеет сложность O(N²), поэтому нужен пространственный индекс. Для сбалансированного BVH и непатологического распределения ожидаемая стоимость составляет `O(N log N + K)`, где `K` — число уникальных пар после broad phase; в худшем случае `K = O(N²)`.

Результат необходимо визуализировать с помощью OpenGL:

показать треугольники внутри кубического объёма; подсветить красным все треугольники, участвующие в пересечениях; добавить освещение; реализовать перемещение камеры по сцене.

Алгоритмы пересечения и пространственного поиска необходимо покрыть unit-тестами, включая граничные и вырожденные случаи.

## Проектирование объектной модели

На лекции обсуждалось, что сначала нужно спроектировать предметные типы, операции и инварианты, и только затем переходить к алгоритмам.

Предметные типы:

`Point3` и `Vector3` - по смыслу 3 числа, но имеют разные осмысленные операции

```cpp
struct Point3 {
    double x;
    double y;
    double z;
};

struct Vector3 {
    double x;
    double y;
    double z;
};

Vector3 operator-(Point3 lhs, Point3 rhs);
Point3  operator+(Point3 point, Vector3 displacement);
Point3  operator-(Point3 point, Vector3 displacement);

Vector3 operator+(Vector3 lhs, Vector3 rhs);
Vector3 operator*(Vector3 vector, double scale);

double dot(Vector3 lhs, Vector3 rhs);
Vector3 cross(Vector3 lhs, Vector3 rhs);
double length(Vector3 vector);
double length(Point3 first, Point3 second);
```

Поля публичны, потому что у точки и вектора нет сложного инварианта который нужно защищать.

`Triangle3`

```cpp

class Triangle3 {
public:
    Triangle3(Point3 a, Point3 b, Point3 c);

    const Point3& vertex(std::size_t index) const;
    Aabb3 bounds() const;
    TriangleKind kind(double epsilon) const;

private:
    std::array<Point3, 3> vertices_;
};
```

При этом допускаются вырожденные треугольники:
```cpp
enum class TriangleKind {
    proper,  // высота к самому длинному ребру больше epsilon
    segment, // высота не больше epsilon
    point    // самое длинное ребро не больше epsilon
};
```

`Plane3`
Сохраняем инвариант `length(normal) = 1` в математической модели (с погрешностью округления в реализации), чтобы `signed_distance` возвращал расстояние со знаком.

```cpp
class Plane3 {
public:
    static std::optional<Plane3>
    through(const Triangle3&, const Tolerance&);

    double signed_distance(Point3 point) const;
    const Vector3& normal() const;
    double offset() const;

private:
    Plane3(Vector3 unit_normal, double offset);

    Vector3 unit_normal_;
    double offset_;
};
```

## Контракт пересечения треугольников

`Triangle3` принимает только конечные координаты, модуль которых не превышает `sqrt(std::numeric_limits<double>::max()) / 16`, и иначе выбрасывает `std::invalid_argument`. Вырожденность допустима и определяется с заданным допуском. Публичная операция не зависит от BVH и OpenGL:

```cpp
enum class IntersectionKind { none, point, segment, area };

IntersectionKind classify_intersection(
    const Triangle3& lhs, const Triangle3& rhs,
    const Tolerance& tolerance);

bool intersects(const Triangle3& lhs, const Triangle3& rhs,
                const Tolerance& tolerance);
```

Ссылки означают обязательные существующие объекты без передачи владения. Функции не меняют треугольники и не сохраняют ссылки на них. `intersects` эквивалентен проверке результата `classify_intersection` на неравенство `none`.

Треугольник является замкнутым множеством вместе с границей. Касание вершиной или ребром считается пересечением. Результаты означают: `none` — общих точек нет; `point` — одна точка; `segment` — отрезок ненулевой длины; `area` — копланарная область ненулевой площади.

### Допуск и вырожденность

```cpp
class Tolerance {
public:
    Tolerance(double absolute, double relative);
    double threshold(double scale) const;
};
```

Конструктор принимает только конечные неотрицательные значения и иначе выбрасывает `std::invalid_argument`. Для конечного `scale >= 0` метод возвращает

```text
min(DBL_MAX, absolute + relative * scale)
```

Сумма и произведение в `threshold` вычисляются с насыщением, поэтому метод не создаёт бесконечность. Нормы вычисляются через `std::hypot`, а геометрические предикаты используют масштабирование до умножения. Если реализация всё же не может получить конечный промежуточный результат, публичная операция выбрасывает `std::overflow_error`, а не возвращает геометрическую классификацию.

Для пары треугольников `pair_scale` равен наибольшей длине их шести рёбер, а `epsilon = tolerance.threshold(pair_scale)`. Если наибольшее ребро треугольника не длиннее `epsilon`, треугольник представляет точку в среднем арифметическом трёх вершин. Координаты среднего вычисляются как сумма делённых на три координат в лексикографическом порядке вершин. Иначе, если высота к наиболее длинному ребру не больше `epsilon`, треугольник представляет отрезок между наиболее удалёнными вершинами. Концы упорядочиваются по `(x, y, z)`; при равенстве длин выбирается лексикографически наименьшая пара. В остальных случаях он представляет заполненный треугольник.

Вид результата равен геометрической размерности пересечения этих представлений. Если точное пересечение пусто, но расстояние между представлениями не больше `epsilon`, результат считается касанием `point`. Ненулевые отрезок и площадь не понижаются до другого вида только из-за своего размера.

Операция симметрична, использует один `epsilon` для обоих аргументов и не зависит от порядка их вершин:

```cpp
classify_intersection(a, b, t) == classify_intersection(b, a, t);
```

## Контракт пространственного поиска

`Triangle3::bounds()` служит единственным публичным способом построить `Aabb3` из треугольника. Внутри BVH коробки можно объединять. Коробка содержит покомпонентные минимумы и максимумы вершин; её границы округляются наружу.

```cpp
class Aabb3 {
public:
    bool overlaps(const Aabb3& other, double epsilon) const;
    static Aabb3 unite(const Aabb3& lhs, const Aabb3& rhs);
private:
    friend class Triangle3;
    Aabb3(Point3 min, Point3 max);
    Point3 min_;
    Point3 max_;
};
```

Инвариант:

```text
min.x <= max.x; min.y <= max.y; min.z <= max.z
```

`Aabb3::overlaps(other, epsilon)` принимает конечный неотрицательный `epsilon` и считает пересекающимися интервалы, расстояние между которыми не больше него. Обязательное постусловие broad phase для пары:

```text
classify_intersection(a, b, t) != none
    => a.bounds().overlaps(b.bounds(), t.threshold(pair_scale(a, b)))
```

Поэтому broad phase может вернуть лишние пары, но не может потерять пару, которую точная проверка считает пересечением.

AABB объединяются в BVH. Дерево имеет один корень, не содержит циклов, каждый некорневой узел имеет одного родителя, а каждый узел достижим из корня. Коробка внутреннего узла содержит коробки детей. Каждый лист непуст. Каждый входной `TriangleId` из `[0, triangles.size())` находится ровно в одном листе; других идентификаторов в листьях нет.

Перед обходом вычисляется `scene_epsilon = tolerance.threshold(scene_scale)`, где `scene_scale` — наибольшая длина ребра во всей сцене. Все проверки узлов BVH вызывают `overlaps` с `scene_epsilon`. Поскольку `scene_epsilon >= pair_epsilon`, отсечение узлов не нарушает постусловие broad phase.

Self-collision-обход порождает все пары внутри листа и между листьями, коробки которых пересекаются с `scene_epsilon`. Внутри листа берутся только пары `i < j`; при сравнении ветвей используется единственный канонический порядок узлов. Поэтому кандидат `(min(a, b), max(a, b))`, `a != b`, появляется ровно один раз и не требует отдельной дедупликации.

```cpp
using TriangleId = std::uint32_t;
struct IntersectionReport { std::vector<std::uint8_t> marked; };

IntersectionReport find_intersections(
    const std::vector<Triangle3>& triangles,
    const Tolerance& tolerance);
```

`TriangleId` равен индексу треугольника во входном векторе. Функция не сохраняет ссылки на вектор или его элементы. После поиска `marked.size() == triangles.size()`. Каждый элемент равен `0` или `1`; `marked[id] == 1` тогда и только тогда, когда при переданном `Tolerance` треугольник `id` пересекается хотя бы с одним другим. Цвет и OpenGL-ресурсы в `Triangle3` не хранятся.