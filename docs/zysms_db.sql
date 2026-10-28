-- ============================================================================
-- 中医舌苔智能识别与诊疗管理一体化系统 数据库初始化脚本
-- 数据库名：zysms_db
-- 依据文档：数据库设计文档 .md（E-R 图 + 10 实体表设计）
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
-- ----------------------------------------------------------------------------
CREATE TABLE department (
  dept_id    INT          NOT NULL AUTO_INCREMENT COMMENT '科室ID',
  dept_name  VARCHAR(64)  NOT NULL                COMMENT '科室名称',
  dept_desc  TEXT                                 COMMENT '科室简介',
  leader_id  INT                                  COMMENT '科室负责人ID（须为主任/副主任医师）',
  created_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (dept_id),
  UNIQUE KEY uk_dept_name (dept_name)
) ENGINE = InnoDB COMMENT = '科室表';

-- ----------------------------------------------------------------------------
-- 3. 工作人员表（staff）：医生 / 管理员
-- ----------------------------------------------------------------------------
CREATE TABLE staff (
  staff_id   INT         NOT NULL AUTO_INCREMENT COMMENT '工作人员ID',
  work_no    VARCHAR(10) NOT NULL                COMMENT '工号（6-10位，登录账号）',
  password   VARCHAR(64) NOT NULL                COMMENT '密码（MD5加密）',
  real_name  VARCHAR(32) NOT NULL                COMMENT '姓名',
  role       TINYINT     NOT NULL                COMMENT '角色：1-医生 2-管理员',
  dept_id    INT                                 COMMENT '所属科室ID（医生必填）',
  title      VARCHAR(16)                         COMMENT '职称：主任医师/副主任医师/主治医师/医师',
  age        TINYINT                             COMMENT '年龄',
  phone      VARCHAR(16)                         COMMENT '联系方式',
  status     TINYINT     NOT NULL DEFAULT 1      COMMENT '状态：0-已删除(逻辑删) 1-正常',
  created_at DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
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
-- ----------------------------------------------------------------------------
CREATE TABLE patient (
  patient_id       INT          NOT NULL AUTO_INCREMENT COMMENT '患者唯一ID',
  medical_card_no  VARCHAR(32)  NOT NULL                COMMENT '医保卡号（登录账号）',
  phone            VARCHAR(16)  NOT NULL                COMMENT '手机号',
  password         VARCHAR(64)  NOT NULL                COMMENT '登录密码（MD5加密）',
  real_name        VARCHAR(32)  NOT NULL                COMMENT '真实姓名',
  gender           TINYINT      NOT NULL DEFAULT 0      COMMENT '性别：0-未知 1-男 2-女',
  age              TINYINT                              COMMENT '年龄',
  id_card          VARCHAR(18)                          COMMENT '身份证号（加密存储）',
  allergy_history  TEXT                                 COMMENT '过敏史',
  past_illness     TEXT                                 COMMENT '既往病史',
  is_first_record  TINYINT      NOT NULL DEFAULT 0      COMMENT '是否完成初次建档：0-未建档 1-已建档',
  status           TINYINT      NOT NULL DEFAULT 1      COMMENT '账号状态：0-禁用 1-正常 2-锁定',
  login_fail_count TINYINT      NOT NULL DEFAULT 0      COMMENT '连续登录失败次数',
  lock_until       DATETIME                             COMMENT '锁定截止时间',
  created_at       DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '注册时间',
  updated_at       DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (patient_id),
  UNIQUE KEY uk_medical_card_no (medical_card_no),
  UNIQUE KEY uk_phone (phone),
  UNIQUE KEY uk_id_card (id_card),
  KEY idx_phone (phone)
) ENGINE = InnoDB COMMENT = '患者表';

-- ----------------------------------------------------------------------------
-- 5. 排班表（schedule）
--    业务规则3.1：同一医生同一天同一班次仅一条有效排班（应用层插入前 COUNT 校验）
-- ----------------------------------------------------------------------------
CREATE TABLE schedule (
  schedule_id      INT      NOT NULL AUTO_INCREMENT COMMENT '排班ID',
  doctor_id        INT      NOT NULL                COMMENT '医生ID',
  work_date        DATE     NOT NULL                COMMENT '值班日期',
  shift_type       TINYINT  NOT NULL                COMMENT '班次：1-上午班(08:00-12:00) 2-下午班(14:00-17:00) 3-晚班(19:00-21:00)',
  max_appointments INT      NOT NULL DEFAULT 20     COMMENT '该时段最大接诊数',
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
  image_url     VARCHAR(255)  NOT NULL                COMMENT '舌苔图片存储路径',
  result_type   VARCHAR(16)   NOT NULL                COMMENT '检测结果：灰黑苔/镜面舌/薄白苔/白腻苔/黄腻苔',
  confidence    DECIMAL(5,2)  NOT NULL                COMMENT '置信度 0~100',
  health_status TINYINT       NOT NULL                COMMENT '健康状态：0-健康 1-病体',
  is_severe     TINYINT       NOT NULL DEFAULT 0      COMMENT '是否重症（灰黑苔/镜面舌）：0-否 1-是',
  device_id     VARCHAR(32)                           COMMENT '采集设备编号',
  created_at    DATETIME      NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '检测时间',
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
-- 9. 处方表（prescription）：一条诊疗记录对应多条处方
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
--     说明：设计文档未给出字段明细，按项目手册 3.2.7（基础参数配置）采用
--     键值对设计，便于扩展（备份周期、锁定时长、默认号源等运行参数）
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
--     说明：设计文档未给出字段明细，按项目手册 3.2.5（日志审计模块）设计：
--     操作时间/操作人/操作类型/内容/结果/异常详情，支持按时间与类型筛选
-- ----------------------------------------------------------------------------
CREATE TABLE system_log (
  log_id          INT          NOT NULL AUTO_INCREMENT COMMENT '日志ID',
  operator_type   TINYINT      NOT NULL DEFAULT 4      COMMENT '操作人类型：1-病人 2-医生 3-管理员 4-系统',
  operator_id     INT                                  COMMENT '操作人ID（系统操作为空）',
  operator_name   VARCHAR(32)                          COMMENT '操作人名称',
  operation_type  VARCHAR(32)  NOT NULL                COMMENT '操作类型：登录/预约/排班/日志查询等',
  operation_desc  VARCHAR(512)                         COMMENT '操作内容描述',
  operation_result TINYINT     NOT NULL DEFAULT 1      COMMENT '操作结果：0-失败 1-成功',
  error_detail    VARCHAR(512)                         COMMENT '异常详情（失败时记录）',
  client_ip       VARCHAR(46)                          COMMENT '客户端IP',
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

-- 测试患者（医保卡号 + 123456 的 MD5）
INSERT INTO patient (medical_card_no, phone, password, real_name, gender, age, is_first_record) VALUES
  ('MP20261024001', '13900000001', 'e10adc3949ba59abbe56e057f20f883e', '张三', 1, 45, 1),
  ('MP20261024002', '13900000002', 'e10adc3949ba59abbe56e057f20f883e', '李四', 2, 32, 0);

-- 示例排班（明天上午/下午班）
INSERT INTO schedule (doctor_id, work_date, shift_type, max_appointments)
SELECT staff_id, CURDATE() + INTERVAL 1 DAY, 1, 20 FROM staff WHERE work_no = '100002';
INSERT INTO schedule (doctor_id, work_date, shift_type, max_appointments)
SELECT staff_id, CURDATE() + INTERVAL 1 DAY, 2, 20 FROM staff WHERE work_no = '100002';

-- 默认系统配置
INSERT INTO system_config (config_key, config_value, remark) VALUES
  ('appointment.lock_threshold', '0', '剩余号源<=该值时锁定时段'),
  ('tongue.valid_confidence', '80', '舌苔结果有效置信度下限(%)'),
  ('account.max_fail_count', '3', '连续登录失败锁定次数'),
  ('account.lock_minutes', '15', '病人端登录锁定时长(分钟)'),
  ('staff.lock_hours', '24', '医生/管理员账号锁定时长(小时)'),
  ('db.backup_cron', '0 2 * * *', '数据库定时备份时间(每天凌晨2点)');
