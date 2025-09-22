extends Area2D



const SCREEN_WIDTH = 320
@export var MOVE_SPEED := 500
var DIRECTION := Vector2.ZERO

func _process(delta):
	position += DIRECTION * MOVE_SPEED * delta
	if position.x < -8 or position.x > SCREEN_WIDTH + 8 :   
		queue_free()


func _on_shot_area_entered(area):
	if area.is_in_group("asteroid") or area.is_in_group("low_nemy"):
		queue_free()
		
