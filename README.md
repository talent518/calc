# calc: C语言实现的可编程(解释执行方式)的运算器
  * flex v2.5.35
  * bison v2.4.1

# v1.1开发计划
  * 数据结构体(OK)
  * 语法分析器(OK): 紧检查语法、生成语法数据(包括前者)。
  * 代码优化器: 把能推导出结果的用户函数、语句列表、语句和表达式(OK)给优化掉。
  * 解释执行器(OK): 优化代码以提升执行效率。
  * 函数库扩展: 基于C/C++实现扩展函数库(OK)，还要增加指针类型(注册资源类型以便于自动回收)，使用PTR_T。
