-- ============================================================================
-- 中医舌苔智能识别与诊疗管理一体化系统 数据库初始化脚本
-- 数据库名：zysms_db
-- 依据文档：数据库设计文档.md（E-R 图 + 10 实体表设计）
--           手册符合性对照表.md（终案 #4/#5/#6/#16/#17，2026-10-30）
-- 适用版本：MySQL 8.0
-- 使用方法：mysql -uroot -p < zysms_db.sql   或在客户端中直接执行
-- ============================================================================

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------------------------------------------------------
-- 1. 建库
-- ----------------------------------------------------------------------------
DROP DATABASE IF EXISTS zysms_db;
CREATE DATABASE zysms_db
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_general_ci;
USE zysms_db;

-- ----------------------------------------------------------------------------
-- 2. 科室表（department）
--    说明：staff.dept_id 外键指向本表，本表 leader_id 又指向 staff，
--    存在相互引用，故 leader_id 的外键在本表建完后用 ALTER 补加。
--    status：管理员端科室删除采用有前置约束的逻辑删除（对照表 管理员端终案）。
-- ----------------------------------------------------------------------------
CREATE TABLE department (
  dept_id    INT          NOT NULL AUTO_INCREMENT COMMENT '科室ID',
  dept_name  VARCHAR(64)  NOT NULL                COMMENT '科室名称',
  dept_desc  TEXT                                 COMMENT '科室简介',
  leader_id  INT                                  COMMENT '科室负责人ID（须为主任/副主任医师）',
  status     TINYINT      NOT NULL DEFAULT 1      COMMENT '状态：0-停用(逻辑删，须无在班排班与在职医生) 1-正常',
  created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (dept_id),
  UNIQUE KEY uk_dept_name (dept_name)
) ENGINE = InnoDB COMMENT = '科室表';

-- ----------------------------------------------------------------------------
-- 3. 工作人员表（staff）：医生 / 管理员
--    锁定字段支撑手册 3.2.4"连续3次错误锁定24小时自动解锁"（医生/管理员口径）。
-- ----------------------------------------------------------------------------
CREATE TABLE staff (
  staff_id        INT         NOT NULL AUTO_INCREMENT COMMENT '工作人员ID',
  work_no         VARCHAR(10) NOT NULL                COMMENT '工号（服务端分配6位，登录输入限10位）',
  password        VARCHAR(64) NOT NULL                COMMENT '密码（MD5加密，手册 3.2.2/3.5.3）',
  real_name       VARCHAR(32) NOT NULL                COMMENT '姓名',
  role            TINYINT     NOT NULL                COMMENT '角色：1-医生 2-管理员',
  dept_id         INT                                 COMMENT '所属科室ID（医生必填）',
  title           VARCHAR(16)                         COMMENT '职称：主任医师/副主任医师/主治医师/医师',
  age             TINYINT                              COMMENT '年龄',
  phone           VARCHAR(16)                         COMMENT '联系方式（仅工作人员回显，无登录语义，明文）',
  status          TINYINT     NOT NULL DEFAULT 1      COMMENT '状态：0-已删除(逻辑删) 1-正常 2-锁定',
  login_fail_count TINYINT    NOT NULL DEFAULT 0      COMMENT '连续登录失败次数',
  lock_until      DATETIME                            COMMENT '锁定截止时间（24小时，见 system_config staff.lock_hours）',
  created_at      DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at      DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (staff_id),
  UNIQUE KEY uk_work_no (work_no),
  KEY idx_dept (dept_id),
  CONSTRAINT fk_staff_dept FOREIGN KEY (dept_id) REFERENCES department (dept_id)
) ENGINE = InnoDB COMMENT = '工作人员表（医生/管理员）';

-- 补加科室负责人外键（解除相互引用）
ALTER TABLE department
  ADD CONSTRAINT fk_dept_leader FOREIGN KEY (leader_id) REFERENCES staff (staff_id);

-- ----------------------------------------------------------------------------
-- 4. 患者表（patient）
--    敏感字段双列方案（对照表 终案 #5/#6）：
--    手机号/身份证号 = MD5 摘要列（字面满足手册 3.2.2"MD5加密存储"，承担唯一约束
--    与登录等值检索）+ AES-256-CBC 密文列（承载展示还原：医生端问诊卡完整手机号
--    ——手册 3.4.4 图例；身份证按 3.4.2 图例脱敏下发）。AES 密钥不入库，
--    由服务端从基础参数 txt（手册 3.2.7）读取。密码维持纯 MD5（终案 #4）。
-- ----------------------------------------------------------------------------
CREATE TABLE patient (
  patient_id       INT          NOT NULL AUTO_INCREMENT COMMENT '患者唯一ID',
  medical_card_no  VARCHAR(32)  NOT NULL                COMMENT '医保卡号（登录主账号，手册表3）',
  phone_hash       CHAR(32)     NOT NULL                COMMENT '手机号 MD5 摘要（唯一约束/手机号登录检索）',
  phone_cipher     VARCHAR(128)                         COMMENT '手机号 AES 密文（Base64，展示时服务端解密）',
  password         VARCHAR(64)  NOT NULL                COMMENT '登录密码（MD5加密）',
  real_name        VARCHAR(32)  NOT NULL                COMMENT '真实姓名',
  gender           TINYINT      NOT NULL DEFAULT 0      COMMENT '性别：0-未知 1-男 2-女',
  age              TINYINT                               COMMENT '年龄',
  id_card_hash     CHAR(32)                             COMMENT '身份证号 MD5 摘要（唯一约束）',
  id_card_cipher   VARCHAR(128)                         COMMENT '身份证号 AES 密文（Base64，脱敏展示用）',
  allergy_history  TEXT                                 COMMENT '过敏史',
  past_illness     TEXT                                 COMMENT '既往病史',
  is_first_record  TINYINT      NOT NULL DEFAULT 0      COMMENT '是否完成初次建档：0-未建档 1-已建档',
  status           TINYINT      NOT NULL DEFAULT 1      COMMENT '账号状态：0-禁用(注册待审核) 1-正常 2-锁定',
  login_fail_count TINYINT      NOT NULL DEFAULT 0      COMMENT '连续登录失败次数',
  lock_until       DATETIME                             COMMENT '锁定截止时间（15分钟，见 system_config account.lock_minutes）',
  created_at       DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间',
  updated_at       DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (patient_id),
  UNIQUE KEY uk_medical_card_no (medical_card_no),
  UNIQUE KEY uk_phone_hash (phone_hash),
  UNIQUE KEY uk_id_card_hash (id_card_hash)
) ENGINE = InnoDB COMMENT = '患者表';

-- ----------------------------------------------------------------------------
-- 5. 排班表（schedule）
--    booked_count：预约挂号事务的号源扣减落点（手册 3.2.2 事务用例逐字落地，
--    对照表 终案 #16）：SELECT ... FOR UPDATE 锁行 → 校验 booked_count <
--    max_appointments → UPDATE +1 → INSERT appointment → COMMIT，失败回滚。
-- ----------------------------------------------------------------------------
CREATE TABLE schedule (
  schedule_id      INT      NOT NULL AUTO_INCREMENT COMMENT '排班ID',
  doctor_id        INT      NOT NULL                COMMENT '医生ID',
  work_date        DATE     NOT NULL                COMMENT '值班日期',
  shift_type       TINYINT  NOT NULL                COMMENT '班次：1-上午班(08:00-12:00) 2-下午班(14:00-17:00) 3-晚班(19:00-21:00)',
  max_appointments INT      NOT NULL DEFAULT 20     COMMENT '该时段最大接诊数',
  booked_count     INT      NOT NULL DEFAULT 0      COMMENT '已约号源数（事务内 FOR UPDATE 校验扣减）',
  created_at       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  PRIMARY KEY (schedule_id),
  KEY idx_doctor_date_shift (doctor_id, work_date, shift_type),
  CONSTRAINT fk_schedule_doctor FOREIGN KEY (doctor_id) REFERENCES staff (staff_id)
) ENGINE = InnoDB COMMENT = '排班表';

-- 可选加固：如需数据库层面强制排班唯一（防止应用层漏判），取消下行注释
-- ALTER TABLE schedule ADD UNIQUE KEY uk_doctor_date_shift (doctor_id, work_date, shift_type);

-- ----------------------------------------------------------------------------
-- 6. 舌苔检测记录表（tongue_record）
--    confidence 采用百分制 0~100（DECIMAL(5,2)）
-- ----------------------------------------------------------------------------
CREATE TABLE tongue_record (
  record_id     INT           NOT NULL AUTO_INCREMENT COMMENT '记录ID',
  patient_id    INT           NOT NULL                COMMENT '患者ID',
  image_url     VARCHAR(255)  NOT NULL                COMMENT '舌苔图片存储路径（TCP 上位机桥接上传，对照表 终案 #12）',
  result_type   VARCHAR(16)   NOT NULL                COMMENT '检测结果：灰黑苔/镜面舌/薄白苔/白腻苔/黄腻苔',
  confidence    DECIMAL(5,2)  NOT NULL                COMMENT '置信度 0~100',
  health_status TINYINT       NOT NULL                COMMENT '健康状态：0-健康 1-病体',
  is_severe     TINYINT       NOT NULL DEFAULT 0      COMMENT '是否重症（灰黑苔/镜面舌）：0-否 1-是',
  device_id     VARCHAR(32)                           COMMENT '采集设备编号（上位机会话绑定，对照表 终案 #13）',
  created_at    DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '检测时间（上位机收帧时间戳）',
  PRIMARY KEY (record_id),
  KEY idx_patient_created (patient_id, created_at),
  CONSTRAINT fk_tongue_patient FOREIGN KEY (patient_id) REFERENCES patient (patient_id)
) ENGINE = InnoDB COMMENT = '舌苔检测记录表';

-- ----------------------------------------------------------------------------
-- 7. 预约记录表（appointment）
-- ----------------------------------------------------------------------------
CREATE TABLE appointment (
  appointment_id   INT      NOT NULL AUTO_INCREMENT COMMENT '预约ID',
  patient_id       INT      NOT NULL                COMMENT '患者ID',
  doctor_id        INT      NOT NULL                COMMENT '医生ID',
  dept_id          INT      NOT NULL                COMMENT '科室ID',
  schedule_id      INT      NOT NULL                COMMENT '关联排班ID',
  appointment_date DATE     NOT NULL                COMMENT '预约就诊日期',
  shift_type       TINYINT  NOT NULL                COMMENT '班次：1-上午 2-下午 3-晚班',
  appoint_type     TINYINT  NOT NULL                COMMENT '预约方式：1-自动预约 2-手动预约',
  status           TINYINT  NOT NULL DEFAULT 1      COMMENT '状态：1-已预约 2-已问诊 3-已失约 4-已取消',
  created_at       DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '预约创建时间',
  PRIMARY KEY (appointment_id),
  KEY idx_doctor_date (doctor_id, appointment_date),
  KEY idx_patient_status_date (patient_id, status, appointment_date),
  KEY idx_schedule (schedule_id),
  CONSTRAINT fk_appt_patient  FOREIGN KEY (patient_id)  REFERENCES patient (patient_id),
  CONSTRAINT fk_appt_doctor   FOREIGN KEY (doctor_id)   REFERENCES staff (staff_id),
  CONSTRAINT fk_appt_dept     FOREIGN KEY (dept_id)     REFERENCES department (dept_id),
  CONSTRAINT fk_appt_schedule FOREIGN KEY (schedule_id) REFERENCES schedule (schedule_id)
) ENGINE = InnoDB COMMENT = '预约记录表';

-- ----------------------------------------------------------------------------
-- 8. 诊疗记录表（medical_record）
-- ----------------------------------------------------------------------------
CREATE TABLE medical_record (
  record_id        INT          NOT NULL AUTO_INCREMENT COMMENT '诊疗记录ID',
  appointment_id   INT                                  COMMENT '关联预约ID',
  patient_id       INT          NOT NULL                COMMENT '患者ID',
  doctor_id        INT          NOT NULL                COMMENT '医生ID',
  diagnosis        TEXT         NOT NULL                COMMENT '诊断结果/此次诊疗信息',
  tongue_record_id INT                                  COMMENT '关联舌苔检测ID',
  report_url       VARCHAR(255)                         COMMENT '生成的报告文件路径',
  created_at       DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '诊疗时间',
  PRIMARY KEY (record_id),
  KEY idx_patient (patient_id),
  KEY idx_doctor (doctor_id),
  KEY idx_appointment (appointment_id),
  CONSTRAINT fk_med_appt     FOREIGN KEY (appointment_id)   REFERENCES appointment (appointment_id),
  CONSTRAINT fk_med_patient  FOREIGN KEY (patient_id)       REFERENCES patient (patient_id),
  CONSTRAINT fk_med_doctor   FOREIGN KEY (doctor_id)        REFERENCES staff (staff_id),
  CONSTRAINT fk_med_tongue   FOREIGN KEY (tongue_record_id) REFERENCES tongue_record (record_id)
) ENGINE = InnoDB COMMENT = '诊疗记录表';

-- ----------------------------------------------------------------------------
-- 9. 处方表（prescription）：一条诊疗记录对应多条处方（批量药材子表方案）
-- ----------------------------------------------------------------------------
CREATE TABLE prescription (
  prescription_id INT          NOT NULL AUTO_INCREMENT COMMENT '处方ID',
  record_id       INT          NOT NULL                COMMENT '关联诊疗记录ID',
  medicine_name   VARCHAR(64)  NOT NULL                COMMENT '药材名称',
  dosage          VARCHAR(32)  NOT NULL                COMMENT '剂量',
  usage_method    VARCHAR(128)                         COMMENT '服用方法',
  sort_order      TINYINT      NOT NULL DEFAULT 0      COMMENT '排序号',
  created_at      DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  PRIMARY KEY (prescription_id),
  KEY idx_record (record_id),
  CONSTRAINT fk_presc_record FOREIGN KEY (record_id) REFERENCES medical_record (record_id)
) ENGINE = InnoDB COMMENT = '处方表';

-- ----------------------------------------------------------------------------
-- 10. 系统配置表（system_config）
--     说明：手册 3.2.7 基础参数的"业务运行参数"部分（网络/DB连接参数走工程内 txt，
--     见决策清单 D9）；锁定时长/备份策略等均可配（手册 3.2.2"可配置备份周期"）。
-- ----------------------------------------------------------------------------
CREATE TABLE system_config (
  config_id    INT          NOT NULL AUTO_INCREMENT COMMENT '配置ID',
  config_key   VARCHAR(64)  NOT NULL                COMMENT '配置键',
  config_value VARCHAR(255) NOT NULL                COMMENT '配置值',
  remark       VARCHAR(255)                         COMMENT '备注说明',
  updated_at   DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (config_id),
  UNIQUE KEY uk_config_key (config_key)
) ENGINE = InnoDB COMMENT = '系统配置表';

-- ----------------------------------------------------------------------------
-- 11. 系统日志表（system_log）
--     说明：按手册 3.2.5（日志审计模块）设计；双轨制另一轨为按日分文件的服务端
--     运行/异常日志（保留90天），本表承载业务操作日志供管理端筛选/导出（终案 #15）。
-- ----------------------------------------------------------------------------
CREATE TABLE system_log (
  log_id          INT          NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  operator_type   TINYINT      NOT NULL DEFAULT 4      COMMENT '操作人类型：1-病人 2-医生 3-管理员 4-系统',
  operator_id     INT                                   COMMENT '操作人ID（系统操作为空）',
  operator_name   VARCHAR(32)                           COMMENT '操作人名称',
  operation_type  VARCHAR(32)  NOT NULL                COMMENT '操作类型：登录/预约/排班/日志查询等',
  operation_desc  VARCHAR(512)                         COMMENT '操作内容描述',
  operation_result TINYINT     NOT NULL DEFAULT 1      COMMENT '操作结果：0-失败 1-成功',
  error_detail    VARCHAR(512)                         COMMENT '异常详情（失败时记录）',
  client_ip       VARCHAR(46)                           COMMENT '客户端IP',
  created_at      DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '操作时间',
  PRIMARY KEY (log_id),
  KEY idx_created (created_at),
  KEY idx_type_created (operation_type, created_at),
  KEY idx_operator (operator_type, operator_id)
) ENGINE = InnoDB COMMENT = '系统日志表';

SET FOREIGN_KEY_CHECKS = 1;

-- ----------------------------------------------------------------------------
-- 12. 测试数据（联调用，生产环境可整段删除）
--     默认密码统一为 123456 的 MD5：e10adc3949ba59abbe56e057f20f883e
--     摘要列直接用 MySQL MD5() 表达式生成（自文档化）；
--     密文列演示数据置 NULL（上线时由服务端 CryptoUtil 生成，见对照表 终案 #5/#6）。
-- ----------------------------------------------------------------------------
INSERT INTO department (dept_name, dept_desc) VALUES
  ('中医内科', '中医内科诊疗科室'),
  ('针灸推拿科', '针灸推拿诊疗科室');

INSERT INTO staff (work_no, password, real_name, role, dept_id, title, age, phone) VALUES
  ('100001', 'e10adc3949ba59abbe56e057f20f883e', '系统管理员', 2, NULL, NULL, 35, '13800000001'),
  ('100002', 'e10adc3949ba59abbe56e057f20f883e', '张主任', 1, 1, '主任医师', 48, '13800000002'),
  ('100003', 'e10adc3949ba59abbe56e057f20f883e', '李医生', 1, 1, '主治医师', 36, '13800000003'),
  ('100004', 'e10adc3949ba59abbe56e057f20f883e', '王医生', 1, 2, '副主任医师', 42, '13800000004');

-- 设置科室负责人（须为主任/副主任医师）
UPDATE department SET leader_id = (SELECT staff_id FROM staff WHERE work_no = '100002') WHERE dept_name = '中医内科';
UPDATE department SET leader_id = (SELECT staff_id FROM staff WHERE work_no = '100004') WHERE dept_name = '针灸推拿科';

-- 测试患者（医保卡号 + 123456 的 MD5；手机号/身份证以 MD5 摘要入列）
INSERT INTO patient (medical_card_no, phone_hash, password, real_name, gender, age,
                     id_card_hash, is_first_record) VALUES
  ('MP20261024001', MD5('13900000001'), 'e10adc3949ba59abbe56e057f20f883e', '张三', 1, 45,
   MD5('330102199001011234'), 1),
  ('MP20261024002', MD5('13900000002'), 'e10adc3949ba59abbe56e057f20f883e', '李四', 2, 32,
   MD5('330102199202023456'), 0);

-- 示例排班（明天上午/下午班，号源扣减从 0 起）
INSERT INTO schedule (doctor_id, work_date, shift_type, max_appointments)
SELECT staff_id, CURDATE() + INTERVAL 1 DAY, 1, 20 FROM staff WHERE work_no = '100002';
INSERT INTO schedule (doctor_id, work_date, shift_type, max_appointments)
SELECT staff_id, CURDATE() + INTERVAL 1 DAY, 2, 20 FROM staff WHERE work_no = '100002';

-- 默认系统配置
INSERT INTO system_config (config_key, config_value, remark) VALUES
  ('appointment.lock_threshold', '0', '剩余号源<=该值时锁定时段'),
  ('tongue.valid_confidence', '80', '舌苔结果有效置信度下限(%)，与采集端 0x0030 参数校准联动'),
  ('account.max_fail_count', '3', '连续登录失败锁定次数（手册 3.2.4/3.3.1）'),
  ('account.lock_minutes', '15', '病人端登录锁定时长·分钟（手册 3.3.1，提示语原文"5分钟"不改）'),
  ('staff.lock_hours', '24', '医生/管理员账号锁定时长·小时（手册 3.2.4）'),
  ('db.backup_cron', '0 2 * * *', '数据库定时备份时间（手册默认每天凌晨2点）'),
  ('db.backup_retention_days', '30', '备份文件保留天数（手册 3.2.2）'),
  ('db.backup_dir', '/var/backups/zysms', '备份文件存储目录（手册 3.2.2"本地指定目录"）');

-- ----------------------------------------------------------------------------
-- 13. 部署加固（手册 3.2.2"限制数据库访问权限"——上线时执行一次，密码请替换）
--     另在 my.cnf [mysqld] 设置 bind-address=127.0.0.1，业务连接一律走专用账号。
-- ----------------------------------------------------------------------------
-- CREATE USER 'zysms'@'127.0.0.1' IDENTIFIED BY 'CHANGE_ME_strong_pwd';
-- GRANT SELECT, INSERT, UPDATE, DELETE ON zysms_db.* TO 'zysms'@'127.0.0.1';
-- FLUSH PRIVILEGES;
