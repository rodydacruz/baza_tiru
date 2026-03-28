extends Node2D

signal score(points, enemy_id)
signal alert(position, enemy_id)
var enemy_id : int = randi() 
enum Phase {
	APEAR,
	HIDE,
	AlL_IN,
	DEFEATED
	
}
var current_phase = Phase.APEAR
var speed = 30
var health = 1000
var explosion_scene = preload("res://scenes/explosion.tscn")


func _process(delta):
	if(get_parent().is_game_over != true):
		movementByFase(delta,current_phase)
	pass
	


func _on_shot_timer_timeout() -> void:
	shoot()
	
func movementByFase(delta,fase):
	match fase:
		Phase.APEAR:
			#Vector2(SCREEN_WIDTH + 8, randf_range(0,SCREEN_HEIGHT))
			if(position.x > Settings.BOST_ATACK_POSITION):
				position.x = position.x - (delta * speed)
			var player_position = get_parent().getPlayerPosition() 
			position.y = position.y + ((1 if position.y < player_position.y else -1) *  (delta * speed)) 
			pass
		_:
			pass
	
func shoot():
	
	var Bullet = preload("res://scenes/shot.tscn")
	var bullet_instance  = Bullet.instantiate()
	bullet_instance.global_position = Vector2(position.x - 20,position.y)
	bullet_instance.global_rotation = 90
	print("Bullet spawned at: ", bullet_instance.global_position)
	bullet_instance.DIRECTION = Vector2.LEFT
	get_parent().add_child(bullet_instance)
	
	
func startHitAnimation():
	pass

func sprintOffScreen():
	pass

func expload(position):
	var stage_node = get_parent()
	
	var explosion_instance = explosion_scene.instantiate()
			
	explosion_instance.position = position
	stage_node.add_child(explosion_instance)
	
func _on_area_entered(area: Area2D) -> void:
	if area.is_in_group("shot"):
		if health > 0:
			emit_signal("score", 10, enemy_id)
			health -= 10
			startHitAnimation()
			expload(area.position)
		
		else:
			expload(area.position)
			#queue_free()
			
