extends Area2D

# Declare member variables here. Examples:
# var a = 2
# var b = "text"]
const MOVE_SPEED = 150.0
const SCREEN_WIDTH = 320
const SCREEN_HEIGHT = 180


var shot_scene = preload("res://scenes/shot.tscn")
var explosion_scene = preload("res://scenes/explosion.tscn")

var can_shot = true

signal destroyed


# Called when the node enters the scene tree for the first time.
func _process(delta):
	
	var input_dir = Vector2()
	
	if Input.is_key_pressed(KEY_UP):
		input_dir.y -= 1.0
	if Input.is_key_pressed(KEY_DOWN):
		input_dir.y += 1.0
	if Input.is_key_pressed(KEY_LEFT):
		input_dir.x -= 1.0
	if Input.is_key_pressed(KEY_RIGHT):
		input_dir.x += 1.0
	if Input.is_key_pressed(KEY_SPACE) and can_shot:
		var stage_node = get_parent()
		
		var shot_instance1 = shot_scene.instantiate()
		var shot_instance2 = shot_scene.instantiate()
		shot_instance1.position = Vector2(position.x + 16,position.y - 5)
		shot_instance1.DIRECTION = Vector2.RIGHT
		shot_instance2.position = Vector2(position.x + 16,position.y + 5)
		shot_instance2.DIRECTION = Vector2.RIGHT
		stage_node.add_child(shot_instance1)
		stage_node.add_child(shot_instance2)
		can_shot = false
		get_node("reload_timer").start()
	
	
	position += (delta * MOVE_SPEED) * input_dir
	
	if position.x < 0.0:
		position.x = 0.0
	if position.x > SCREEN_WIDTH:
		position.x = SCREEN_WIDTH
	if position.y < 0.0:
		position.y = 0.0
	if position.y > SCREEN_HEIGHT:
		position.y = SCREEN_HEIGHT
		
	


# Called every frame. 'delta' is the elapsed time since the previous frame.
#func _process(delta):
#	pass


func _on_reload_timer_timeout():
	can_shot = true
	
func _getLineToVector(vector2):
	
	pass
func _getAngleToVector(vector2):
	pass

func _on_player_area_entered(area):
	if area.is_in_group("asteroid",) or area.is_in_group("shot") or area.is_in_group("canon_ball"):
		var stage_node = get_parent()
		
		var explosion_instance = explosion_scene.instantiate()
		explosion_instance.position = position
		stage_node.add_child(explosion_instance)
		emit_signal("destroyed")
		queue_free()
		
